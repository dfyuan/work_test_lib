#ifndef AITXUDEF_H_INCLUDED
#define AITXUDEF_H_INCLUDED
#include <linux/version.h>
#include <linux/videodev2.h>
//namespace android {
//#define DbgMsg
#define DbgMsg _PrintDebugging_
int _PrintDebugging_(const char *format, ...);

//#define DEBUG_MSG 1
/*
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
//#define USE_UVCIOC_CTRL_QUERY 1//for 3.2.x kernel or after , please set this define as 1, for old kernel please set 0
#else
//#define USE_UVCIOC_CTRL_QUERY 0
#endif
*/
#if USE_UVCIOC_CTRL_QUERY
//UVC control signals
#define UVC_SET_CUR  0x01
#define UVC_GET_CUR  0x81
#define UVC_GET_MIN  0x82
#define UVC_GET_MAX  0x83
#endif

//function returns
#define SUCCESS 0
#define FAILED 0x80000003
#define AITAPI_DEV_NOT_EXIST 0x8000000
#define AITAPI_NOT_SUPPORT 0x80000001
#define NOT_ENOUGH_MEM 0x80000002

//bmRequestType, usb spec 2.0 page 248
#define REQUEST_TYPE_CLASS_SET_CUR 0x20   //class SET_CUR
#define REQUEST_TYPE_CLASS_GET_CUR 0xA0   //class GET_CUR
#define REQUEST_TYPE_VENDOR_SET_CUR 0x40   //vendor SET_CUR
#define REQUEST_TYPE_VENDOR_GET_CUR 0xC0   //vendor GET_CUR

//bmRequest, usb spec 2.0 page 248, uvc 1.1 spec 137
#define SET_CUR 0x01
#define GET_CUR 0x81
#define GET_LEN 0x85

//wValue,interface
#define AIT_XU_UNIT_ID              0x0600  //for wValue
#define XU_UNIT_ID_AIT_XU           0x06    //for UVC unit id

//wIndex, Control Selector
#define EXU1_CS_SET_ISP				0x01
#define EXU1_CS_GET_ISP_RESULT		0x02
#define EXU1_CS_SET_FW_DATA			0x03
#define EXU1_CS_SET_MMP				0x04
    #define EXU1_MMP_LED_CONTROL        0x0E
    #define EXU1_MMP_TRIGGER_MJPG     0x12
#define EXU1_CS_GET_MMP_RESULT		0x05
#define EXU1_CS_SET_ISP_EX			0x06
#define EXU1_CS_GET_ISP_EX_RESULT	0x07
#define EXU1_CS_READ_MMP_MEM		0x08
#define EXU1_CS_WRITE_MMP_MEM		0x09
//#define EXU1_CS_GET_CHIP_INFO		0x0A
#define EXU1_CS_GET_DATA_32         0x0B
#define EXU1_CS_GET_DATA	        0x0B    //same EXU1_CS_GET_DATA_32 but size change to 30 bytes
    #define EXU1_CS_GET_DATA_LEN    32//30      //XU data size
#define EXU1_CS_CROP_IMAGE          0x0D //arthur@20141030
#define EXU_CS_SET_MMP_CMD16        0x0E //arthur@20141104
#define EXU1_CS_SET_MMP_CMD16       0x0E //arthur@20141104
    #define EXU1_MMP16_SET_TV_PATAM     0x01    //sub command
    #define EXU1_MMP16_SET_TV_OPTION    0x02    //sub command
    #define EXU1_MMP16_TRIGGER_MJPG     0x03    //sub command
    #define EXU1_MMP16_ENABLE_M2TS      0x06
    #define EXU1_MMP16_ENABLE_VR        0x07
    #define EXU1_MMP16_ENABLE_CROP      0x08


#define EXU_CS_GET_MMP_RESULT16     0x0F
#define EXU1_CS_GET_MMP_RESULT16    0x0F

//sub command ID for EXU1_CS_SET_ISP (BYTE0)
#define EXUID_DO_AF                 0
#define EXUID_SET_FPS               2
#define EXUID_SET_RESOLUTION        3
#define EXUID_SET_IFRAME            4
#define EXUID_SET_FLICKMODE         9
#define EXUID_SET_VIDEO_QUALITY     8
#define EXUID_SET_REG               10
    //sub command for EXUID_SET_REG (BYTE1)
    #define REG_READ        0x00
    #define REG_WRITE       0x00
    #define SENSOR_READ     0x02        //read sensor i2c
    #define SENSOR_WRITE    0x03        //write sensor i2c
#define EXUID_FW_BURNING            1
#define EXUID_GET_FW_VER            11
#define EXUID_GET_FW_BUILDDAY       12
#define EXUID_SET_BITRATE           8
#define EXUID_SET_H264MODE          13
#define EXUID_SET_WDR               14
#define EXUID_DEVICE_MEM_ACCESS     15
	#define DEVICE_READ_BYTE    0   //sub command for EXUID_DEVICE_MEM_ACCESS
	#define DEVICE_WRITE_BYTE   1
	#define DEVICE_READ_WORD    2
	#define DEVICE_WRITE_WORD   3
	#define DEVICE_READ_DWORD   4
	#define DEVICE_WRITE_DWORD  5

//sub command ID for EXU1_CS_SET_MMP (BYTE0)
#define EXU_MMP_DOWNLOAD_MMP_FW_CMD		0x01
#define EXU_MMP_GET_DRAM_SIZE			0x02
#define EXU_MMP_GET_FLASH_ID			0x03
#define EXU_MMP_GET_FLASH_SIZE			0x04
#define EXU_MMP_ERASE_FLASH				0x05
#define EXU_MMP_CHECKSUM_RESULT			0x06
#define EXU_MMP_SET_MAX_QP				0x07
#define EXU_MMP_SET_MIN_QP				0x08
#define EXU_MMP_SET_P_COUNT				0x09
#define EXU_MMP_RW_DATA_PARAM			0x0A
    #define MMP_GET_FD_POS              0x02    //sub command
#define EXU_MMP_ENCODING_RESOLUTION		0x0B
#define EXU_MMP_SKYPE_MODE				0x0C
#define EXU_MMP_STREAM_MODE				0x0C



//sub command ID for set MMP command16
#define EXUID_SET_SECTV_PREVIEW_PARAM   0x01

//////////////////////////////////FOR UVC 1.1 SPEC///////////////////////////////
// camera terminal control selectors
#define  CT_CONTROL_UNDEFINED                  0x00
#define  CT_SCANNING_MODE_CONTROL              0x01
#define  CT_AE_MODE_CONTROL                    0x02
#define  CT_AE_PRIORITY_CONTROL                0x03
#define  CT_EXPOSURE_TIME_ABSOLUTE_CONTROL     0x04
#define  CT_EXPOSURE_TIME_RELATIVE_CONTROL     0x05
#define  CT_FOCUS_ABSOLUTE_CONTROL             0x06
#define  CT_FOCUS_RELATIVE_CONTROL             0x07
#define  CT_FOCUS_AUTO_CONTROL                 0x08
#define  CT_IRIS_ABSOLUTE_CONTROL              0x09
#define  CT_IRIS_RELATIVE_CONTROL              0x0A
#define  CT_ZOOM_ABSOLUTE_CONTROL              0x0B
#define  CT_ZOOM_RELATIVE_CONTROL              0x0C
#define  CT_PANTILT_ABSOLUTE_CONTROL           0x0D
#define  CT_PANTILT_RELATIVE_CONTROL           0x0E
#define  CT_ROLL_ABSOLUTE_CONTROL              0x0F
#define  CT_ROLL_RELATIVE_CONTROL              0x10
#define  CT_PRIVACY_CONTROL                    0x11

#if 1   //for UVC V4L2
#define CAMERA_ZOOM         V4L2_CID_ZOOM_ABSOLUTE
#define CAMERA_PAN          V4L2_CID_PAN_ABSOLUTE
#define CAMERA_TILT         V4L2_CID_TILT_ABSOLUTE
#define CAMERA_FOCUS        V4L2_CID_FOCUS_ABSOLUTE
#endif

#if 0//for Ap
#define CAMERA_ZOOM     1
#define CAMERA_PAN      2
#define CAMERA_TILT     3
#define CAMERA_FOCUS    4
#endif
////////////////////////////////////////////////////////////////////////////////////

//for PCSYNC interface
#define AIT_PCSYNC_1                    0x04    //interface 4, with ep1 for bulk transfer
#define AIT_PCSYNC_2                    0x05    //ineerface 5, control transfer only
#define PCSYNC_MAX_PACKET_SIZE          512
#define PCSYNC_MAX_TRANSFER_SIZE        (1024*64)
#define PCSYNC_BULK_OUT_EP              0x01
#define PCSYNC_BULK_IN_EP               0x81

//PCSYNC vendor request
#define PCSYNC_CTRL_BULK_IN				0x81
#define PCSYNC_CTRL_BULK_OUT			0x82
#define PCSYNC_VENDER_CTRL_IN_TEST 		0x83
#define PCSYNC_VENDER_CTRL_OUT_TEST 	0x84
#define PCSYNC_GET_FD_LIST				0x01
#define PCSYNC_SET_FD_PARAM				0x02
#define PCSYNC_BURN_FIRMWARE			0x03
#define PCSYNC_PARTIAL_FLASH_RW         0x04


//};
#endif // AITXUDEF_H_INCLUDED

