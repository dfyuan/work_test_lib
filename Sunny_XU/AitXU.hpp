#ifndef AITXU_H
#define AITXU_H

#include <linux/videodev2.h>
#include <stdio.h>
#include <stdint.h>

//returns
#define AIT_XU_OK                   0x00000000
#define AIT_H264_DUMMY_FRAME        0x00000001
#define AIT_H264_IFRAME             0x00000002
#define AIT_H264_PFRAME             0x00000003

#define AIT_XU_ERROR                0x80000000
#define AIT_XU_OUT_OF_RANGE         0x80000001

//XU command directon from HOST side
#define AIT_XU_CMD_IN  0
#define AIT_XU_CMD_OUT 1

#define PTZ_PAN_STEP    0xE10
#define PTZ_TILT_STEP   0xE10
#define PTZ_ZOOM_STEP   0x01

#define VID_MIRROR  (1<<0)
#define VID_FLIP    (1<<1)
#define VID_WOV     (1<<2)
#define VID_ENABLE_FD   (1<<3)

#pragma pack(1)
typedef struct _FACE_CMD{
    unsigned char cmd;
    unsigned char dlen;
    unsigned char d[30];
}FACE_CMD;

typedef struct _FaceRect{
    unsigned short left;
    unsigned short top;
    unsigned short right;
    unsigned short bottom;
}FaceRect;

typedef struct _FacePositionList{
    unsigned short num_face;
    FaceRect face_rect[20];
}FacePositionList;

typedef void* AitXUHandle;

/*
//Ait extension unit initialize
//This function must be called before using any SkypeXU functions.
//parameters:
//[IN] dev: V4L2 video device handle, The caller gets this handle by calling fopen with desired device name, then pass the handle to this api.
//return: reutrn AitXU api handle
AitXUHandle SkypeXU_Init(int dev);

//release AitXUHandle
//release all resource that allocated by SkypeXU_Init
//parameter:
//[IN] AitXUHandle *handle: address of AitXUHandle
void SkypeXU_Release(AitXUHandle *handle);

//int SkypeXU_SetMode(AitXUHandle handle,uint8_t mode);
//int SkypeXU_SetEncRes(AitXUHandle handle,uint8_t resolution);

//Set H264 stream bitrate
//parameters:
//[IN] AitXUHandle handle:
//[IN] int birrate: set H264 bitrate in kbps
int SkypeXU_SetBitrate(AitXUHandle handle,int bitrate);

//Set H264 frame rate
//parameters:
//[IN] AitXUHandle handle:
//[IN] uint8_t fps: set H264 frame rate in frame per second.
int SkypeXU_SetFrameRate(AitXUHandle handle,uint8_t fps);

//Force device send an H264 IDR frame to H264 stream immediately.
//parameters:
//[IN] AitXUHandle handle:
int SkypeXU_SetIFrame(AitXUHandle handle);
*/
extern AitXUHandle AitXU_Init(char *dev_name);
extern AitXUHandle AitXU_Init(const char *dev_name);
AitXUHandle AitXU_Init_from_handle(int dev);
void AitXU_Release(AitXUHandle* handle);
void AitXU_Release2(AitXUHandle* handle);
int AitXU_IspCmd(AitXUHandle handle,uint8_t *cmd_in,uint8_t *cmd_out);
//Set H264 stream ID for setting. This function didn't be supportted if camera only support single stream.
//[IN] id: Selected ID which you would like to control.
extern int AitXU_SetSelectStreamID4Setting(AitXUHandle handle,int id);

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

//Set H264 encoding resolution
int AitXU_SetEncRes(AitXUHandle handle,uint8_t resolution);

//Force device send an H264 IDR frame to H264 stream immediately.
//parameters:
//[IN] AitXUHandle handle:
int AitXU_SetIFrame(AitXUHandle handle);

int AitXU_XuCmd(AitXUHandle handle,unsigned char* cmd,unsigned short cs,unsigned char len, unsigned char direction);

int AitXU_WriteSensorReg(AitXUHandle handle,unsigned short addr, unsigned short val);
int AitXU_ReadSensorReg(AitXUHandle handle,unsigned short addr, unsigned short *val);

int AitXU_WriteReg(AitXUHandle handle,unsigned short addr, unsigned char val);
int AitXU_ReadReg(AitXUHandle handle,unsigned short addr, unsigned char *val);

//Standard UVC Camera unit control
int AitUVC_CameraCmd(AitXUHandle handle,__s64 *val,__u32 id,unsigned char direction);

//get current video options from device
//[IN] AitXUHandle handle:
//[OUT] unsigned int *options: address of 4byte buffer to receive options
int AitXU_GetVideoOption(AitXUHandle handle,unsigned int *options);

//set video options to device
//[IN] AitXUHandle handle:
//[OUT] unsigned int options:video options aS below
// VID_MIRROR : Enable video mirror
// VID_FLIP : Enable video flip
int AitXU_SetVideoOption(AitXUHandle handle, unsigned int options);

//read codec register
//[IN] AitXUHandle handle:
//[IN] unsigned sjort : 16 bit address
//[OUT] unsigned char *val : return register value
int AitXU_ReadCodecReg(AitXUHandle handle, unsigned short addr, unsigned char *val);

//write codec register
//[IN] AitXUHandle handle:
//[IN] unsigned short addr: address
//[IN] unsigned char val: value
int AitXU_WriteCodecReg(AitXUHandle handle, unsigned short addr, unsigned char val);

//enable/disable face detection
//[IN] AitXUHandle handle:
//[IN] unsigned char enable: Enable=1, Disable=0
int AitXU_EnableFD(AitXUHandle handle,unsigned char enable);


int AitXU_GetFD_Position(AitXUHandle handle,FacePositionList *face_pos_list);

//trigger a MJPG snapshot, this function can only available when SIGNAL TYPE = 3 or 4
int AitXU_TriggerMJPG(AitXUHandle handle, unsigned char Jcount, unsigned char Jinterval);
int AitXU_EnableM2TS(AitXUHandle handle,unsigned char enable);
int AitXU_EnableVR(AitXUHandle handle,unsigned char enable);
int AitXU_EnableCROP(AitXUHandle handle,unsigned char enable,unsigned short crop_startX, unsigned short crop_startY);

int AitXU_SetPFrameCount(AitXUHandle handle,unsigned int val);
//int AitUVC_QuryCtrls(AitXUHandle handle);

//UVC
//UVC Processing Unit control
//[IN] AitXUHandle handle:
//[IN/OUT] __s32 *val:
//                      V4L2_CID_BRIGHTNESS	integer	Picture brightness, or more precisely, the black level.
//                      V4L2_CID_CONTRAST	integer	Picture contrast or luma gain.
//                      V4L2_CID_SATURATION	integer	Picture color saturation or chroma gain.
//                      V4L2_CID_HUE	integer	Hue or color balance.
//[IN] unsigned char direction: Command direction,
//                      AIT_XU_CMD_IN: Get value to host
//                      AIT_XU_CMD_OUT: Set value to device
int AitUVC_PUControl(AitXUHandle handle,__s32 *val,__u32 id,unsigned char direction); //processing unit control
int AitUVC_PUQuery(AitXUHandle handle,__u32 id,__s32 *max,__s32 *min,__s32 *step);

int AitXU_WriteFWData(AitXUHandle handle,unsigned char* data,unsigned int Len);
int AitXU_UpdateFW(AitXUHandle handle,unsigned char* fw_data,unsigned int len);
int AitXU_GetFWBuildDate(AitXUHandle handle,char *fwBuildDay);
int AitXU_GetFWVersion(AitXUHandle handle,unsigned char *fwVer);
int AitXU_UpdateFW_843X(AitXUHandle handle,unsigned char* fw_data,unsigned int len);

//for KT only
int AitXU_GetFWVersion(AitXUHandle handle,unsigned char *fwVer);
int AitXU_UpdateAudioData(AitXUHandle handle, unsigned char *audio_data, unsigned long len);
int AitXU_AudioPlayOp(AitXUHandle handle, unsigned char operation, unsigned short volume);

//Ait isp command extension (16bytes)
//[IN] AitXUHandle handle:
//[IN] uint8_t *cmd_in: 16 bytes command buffer send to device
//[OUT] uint8_t *cmd_out: 16 bytes result receive from device
int AitXU_IspExCmd(AitXUHandle handle,uint8_t *cmd_in,uint8_t *cmd_out);
int AitXU_Mmp16Cmd(AitXUHandle handle,uint8_t *cmd_in,uint8_t *cmd_out);
int AitXU_MmpCmd(AitXUHandle handle,uint8_t *cmd_in,uint8_t *cmd_out);
int AitXU_SetCropControl(   AitXUHandle handle
                        ,   uint16_t start_x, uint16_t start_y
                        ,   uint16_t src_w, uint16_t src_h
                        ,   uint16_t dest_w, uint16_t dest_h);


int AitXU_GetCropControl(  AitXUHandle handle
                        ,   uint16_t *start_x, uint16_t *start_y
                        ,   uint16_t *src_w, uint16_t *src_h
                        ,   uint16_t *dest_w, uint16_t *dest_h);

int AitXU_Request_SensorResolution(AitXUHandle handle);
int AitXU_Get_SensorResolution(  AitXUHandle handle
                        , uint16_t *sensor_w, uint16_t *sensor_h);
int AitXU_SetMinFps(AitXUHandle handle,uint8_t Min_Fps);
int AitXU_RequestLux(AitXUHandle handle);
int AitXU_GetLux(AitXUHandle handle,uint16_t* Lux);

int AitXU_SetShutter(AitXUHandle handle,uint16_t Shutter);
int AitXU_SetGain(AitXUHandle handle,uint16_t Gain);
int AitXU_SetLog(AitXUHandle handle,uint8_t Log);

extern int AitXU_MmpMemCmd(AitXUHandle handle,uint8_t *cmd_in,uint8_t *cmd_out);

int AitXU_GetFlashType(AitXUHandle handle,uint8_t *cmd_out);
int AitXU_GetFlashSize(AitXUHandle handle,uint8_t *cmd_out);
int AitXU_GetFlashErase(AitXUHandle handle,unsigned int addr);

int AitXU_SetWDR(AitXUHandle handle,unsigned int val);
#endif
