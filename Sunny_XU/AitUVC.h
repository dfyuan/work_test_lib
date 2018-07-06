#ifndef AITUVC_H_INCLUDED
#define AITUVC_H_INCLUDED
#include <stdint.h>

#define SUB_H264_1280X720 0x09
#define SUB_H264_640X480 0x00
#define SUB_H264_320X240 0x03
#define SUB_H264_160X120 0x01

#define SKYPE_H264_MODE 2

typedef void* AitXUHandle;
#define SKYPE_XU_OK                 0x00000000
#define AIT_H264_DUMMY_FRAME        0x00000001
#define AIT_H264_IFRAME             0x00000002
#define AIT_H264_PFRAME             0x00000003

#define SKYPE_XU_ERROR              0x80000000
#define SKYPE_XU_OUT_OF_RANGE       0x80000001

typedef enum {
  
  STREAM_MODE_FRAMEBASE_H264 =0x81,
  STREAM_MODE_FRAMEBASE_DUAL_H264 =0x86,
  STREAM_MODE_FRAMEBASE_H264L_MJPEG =0x88
}AIT_STREAM_MODE;
	
extern void UVC_SetUVCKernelVersion(__u32 version);
extern __u32 UVC_GetUVCKernelVersion(void);

AitXUHandle SkypeXU_Init(int dev);
void SkypeXU_Release(AitXUHandle *handle);
extern int AitXU_SetMode(AitXUHandle handle,AIT_STREAM_MODE mode);
//Set H264 encoding resolution
extern int AitXU_SetEncRes(AitXUHandle handle,uint8_t resolution);

int SkypeXU_SetEncRes(AitXUHandle handle,uint8_t resolution);
int SkypeXU_SetBitrate(AitXUHandle handle,int bitrate);
int SkypeXU_SetFrameRate(AitXUHandle handle,uint8_t fps);
int SkypeXU_SetIFrame(AitXUHandle handle);
int AitH264Demuxer(void* handle,uint8_t *buffer,uint8_t **ppH264,uint32_t *pH264Len,uint8_t **ppYUV,uint32_t *pYUVLen);
int AitH264Demuxer_SkypeModeB(uint8_t *buffer,uint8_t **ppH264,uint32_t *pH264Len,uint8_t **ppYUV,uint32_t *pYUVLen);
int AitH264Demuxer_MJPG_H264(const uint8_t* src_buf, uint32_t src_buf_len, uint8_t* h264_buf, uint32_t h264_buf_len, uint32_t *h264_buf_used);
extern void UVC_SetUVCKernelVersion(__u32 version);

AitXUHandle AitXU_Init(char *dev_name);

void AitXU_Release(AitXUHandle* handle);
//Set H264 stream bitrate
//parameters:
//[IN] AitXUHandle handle:
//[IN] int birrate: set H264 bitrate in kbps
int AitXU_SetBitrate(AitXUHandle handle,int bitrate);

//Set H264 frame rate
//parameters:
//[IN] AitXUHandle handle:
//[IN] uint8_t fps: set H264 frame rate in frame per second.
int AitXU_SetFrameRate(AitXUHandle handle,uint8_t fps);

//Force device send an H264 IDR frame to H264 stream immediately.
//parameters:
//[IN] AitXUHandle handle:
int AitXU_SetIFrame(AitXUHandle handle);


#endif // AITUVC_H_INCLUDED
