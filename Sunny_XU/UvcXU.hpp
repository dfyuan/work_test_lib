#ifndef UVCXU_H
#define UVCXU_H
//#include <linux/types.h>
//#include <time.h>
#include <sys/time.h>
#include <linux/videodev2.h>



#include <sys/ioctl.h>

//#include <sys/ioctl.h>
//#include <sys/time.h>


#include <errno.h>
#include <errno.h>
#include "AitUVC.h"
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
//#include <linux/uvcvideo.h>

#include "AitXuDef.hpp"
//#define USE_UVCIOC_CTRL_QUERY 0 //for 3.2.x kernel or after , please set this define as 1, for old kernel please set 0

//namespace android {
#if USE_UVCIOC_CTRL_QUERY
extern "C" {
//#include <linux/usb/video.h>


#ifdef __CHECKER__
# define __user __attribute__((noderef, address_space(4)))
# define __kernel /* default address space */
#else
# define __user
# define __kernel
#endif

#pragma pack(4)
//#include <linux/uvcvideo.h>

}
#else

//#include <linux/usb/video.h>

#ifdef __CHECKER__
# define __user __attribute__((noderef, address_space(4)))
# define __kernel /* default address space */
#else
# define __user
# define __kernel
#endif

#pragma pack(1)
//#include <linux/uvcvideo.h>

#endif

//#define V4L_CID_SKUPE_XU_SET_ENCRES (V4L_CID_AIT_XU_BASE+2)
//#define V4L_CID_SKUPE_XU_SET_MODE (V4L_CID_AIT_XU_BASE+3)

/*
 * asm-generic/int-ll64.h
 *
 * Integer declarations for architectures which use "long long"
 * for 64-bit types.
 */

///////////////////////// int-ll64.h ///////////////////
#ifndef _ASM_GENERIC_INT_LL64_H
#define _ASM_GENERIC_INT_LL64_H
//#include <asm/bitsperlong.h>
#ifndef __ASSEMBLY__
/*
 * __xx is ok: it doesn't pollute the POSIX namespace. Use these in the
 * header files exported to user space
 */
typedef __signed__ char __s8;
typedef unsigned char __u8;
typedef __signed__ short __s16;
typedef unsigned short __u16;
typedef __signed__ int __s32;
typedef unsigned int __u32;
#ifdef __GNUC__
__extension__ typedef __signed__ long long __s64;
__extension__ typedef unsigned long long __u64;
#else
typedef __signed__ long long __s64;
typedef unsigned long long __u64;
#endif
#endif /* __ASSEMBLY__ */
#endif /* _ASM_GENERIC_INT_LL64_H */
///////////////////////////////////////////////////////
#if 0 //don't use it in 3.3 kernel
#ifdef __CHECKER__
# define __user __attribute__((noderef, address_space(1)))
# define __kernel /* default address space */
#else
# define __user
# define __kernel
#endif
#endif
/* Data types for UVC control data */
#define UVC_CTRL_DATA_TYPE_RAW		0
#define UVC_CTRL_DATA_TYPE_SIGNED	1
#define UVC_CTRL_DATA_TYPE_UNSIGNED	2
#define UVC_CTRL_DATA_TYPE_BOOLEAN	3
#define UVC_CTRL_DATA_TYPE_ENUM		4
#define UVC_CTRL_DATA_TYPE_BITMASK	5

/* Control flags */
#define UVC_CONTROL_SET_CUR	(1 << 0)
#define UVC_CONTROL_GET_CUR	(1 << 1)
#define UVC_CONTROL_GET_MIN	(1 << 2)
#define UVC_CONTROL_GET_MAX	(1 << 3)
#define UVC_CONTROL_GET_RES	(1 << 4)
#define UVC_CONTROL_GET_DEF	(1 << 5)
/* Control should be saved at suspend and restored at resume. */
#define UVC_CONTROL_RESTORE	(1 << 6)
/* Control can be updated by the camera. */
#define UVC_CONTROL_AUTO_UPDATE	(1 << 7)
/* Control is an extension unit control. */
#define UVC_CONTROL_EXTENSION	(1 << 8)

#define UVC_CONTROL_GET_RANGE	(UVC_CONTROL_GET_CUR | UVC_CONTROL_GET_MIN | \
				 UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_RES | \
				 UVC_CONTROL_GET_DEF)

//Camera Unit Control Selector
//#define UVC_CT_PANTILT_ABSOLUTE_CONTROL 0x0D

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
}GUID;
#endif


#define UVC_GUID_UVC_CAMERA \
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}

#define DEBUG_MSG 1
#define V4L_CID_AIT_XU_BASE                     (V4L2_CID_PRIVATE_BASE)
#define V4L_CID_AIT_XU_SET_ISP                  (V4L_CID_AIT_XU_BASE+1)
#define V4L_CID_AIT_XU_GET_ISP_RESULT           (V4L_CID_AIT_XU_BASE+2)
#define V4L_CID_AIT_XU_SET_MMP_CMD16            (V4L_CID_AIT_XU_BASE+3)
#define V4L_CID_AIT_XU_GET_MMP_CMD16_RESULT     (V4L_CID_AIT_XU_BASE+4)
#define V4L_CID_AIT_XU_GET_DATA                 (V4L_CID_AIT_XU_BASE+5)
#define V4L_CID_SKUPE_XU_SET_ENCRES             (V4L_CID_AIT_XU_BASE+6)
#define V4L_CID_SKUPE_XU_SET_MODE               (V4L_CID_AIT_XU_BASE+7)
#define V4L_CID_AIT_XU_SET_MMP                  (V4L_CID_AIT_XU_BASE+8)
#define V4L_CID_AIT_XU_GET_MMP_RESULT           (V4L_CID_AIT_XU_BASE+9)
#define V4L_CID_AIT_XU_SET_FW_DATA              (V4L_CID_AIT_XU_BASE+10)
#define V4L_CID_AIT_XU_SET_ISP_EX               (V4L_CID_AIT_XU_BASE+11)
#define V4L_CID_AIT_XU_GET_ISP_EX_RESULT        (V4L_CID_AIT_XU_BASE+12)
#define V4L_CID_AIT_XU_SET_MMP_MEM              (V4L_CID_AIT_XU_BASE+13)
#define V4L_CID_AIT_XU_GET_MMP_MEM              (V4L_CID_AIT_XU_BASE+14)

#define V4L_CID_AIT_XU_SET_CROP_PARAM0          (V4L_CID_AIT_XU_BASE+15)
#define V4L_CID_AIT_XU_SET_CROP_PARAM1          (V4L_CID_AIT_XU_BASE+16)
#define V4L_CID_AIT_XU_SET_CROP_PARAM2          (V4L_CID_AIT_XU_BASE+17)
#define V4L_CID_AIT_XU_SET_CROP_PARAM3          (V4L_CID_AIT_XU_BASE+18)
#define V4L_CID_AIT_XU_SET_CROP_PARAM4          (V4L_CID_AIT_XU_BASE+19)
#define V4L_CID_AIT_XU_SET_CROP_PARAM5          (V4L_CID_AIT_XU_BASE+20)
#define V4L_CID_AIT_XU_SET_CROP_PARAM6          (V4L_CID_AIT_XU_BASE+21)
#define V4L_CID_AIT_XU_SET_CROP_PARAM7          (V4L_CID_AIT_XU_BASE+22)

#define UVCIOC_CTRL_ADD_2_6_32  _IOW ('U', 1, struct uvc_xu_control_info_2_6_32)
#define UVCIOC_CTRL_MAP_2_6_32  _IOWR('U', 2, struct uvc_xu_control_mapping_2_6_32)
#define UVCIOC_CTRL_MAP_2_6_36  _IOWR('U', 2, struct uvc_xu_control_mapping_2_6_36)
#define UVCIOC_CTRL_GET         _IOWR('U', 3, struct uvc_xu_control)
#define UVCIOC_CTRL_SET         _IOW ('U', 4, struct uvc_xu_control)
#define UVCIOC_CTRL_QUERY_3_2_0	   _IOWR('u', 0x21, struct uvc_xu_control_query_3_2_0)
#define UVCIOC_CTRL_MAP_3_2_0		_IOWR('u', 0x20, struct uvc_xu_control_mapping_3_2_0)
typedef int s32;

struct uvc_xu_control_info_2_6_32 {
        __u8 entity[16];
        __u8 index;
        __u8 selector;
        __u16 size;
        __u32 flags;
};


struct uvc_xu_control {
        __u8 unit;
        __u8 selector;
        __u16 size;
        __u8 *data;
};

struct uvc_xu_control_query_3_2_0 {
          __u8 unit;
          __u8 selector;
          __u16 query;             /* Video Class-Specific Request Code, */
                                  /* defined in linux/usb/video.h A.8.  */
          __u32 size;
          __u8  *data;
};


//Mapping structure used from 2.6.32 - 2.6.35 kernel

struct uvc_xu_control_mapping_2_6_32 {
        __u32 id;
        __u8 name[32];
        __u8 entity[16];
        __u8 selector;

        __u8 size;
        __u8 offset;
        enum v4l2_ctrl_type v4l2_type;
        __u32 data_type;
	
	__u8 reserved[1];
	
};

//Mapping structure used for 2.6.36 to < 3.0
struct uvc_xu_control_mapping_2_6_36 {
        __u32 id;
        __u8 name[32];
        __u8 entity[16];
        __u8 selector;

        __u8 size;
        __u8 offset;
        enum v4l2_ctrl_type v4l2_type;
        __u32 data_type;
        struct uvc_menu_info /*__user*/ *menu_info;
        __u32 menu_count;

        __u32 reserved[4];
};


struct uvc_xu_control_mapping_3_2_0 {
          __u32 id;
          __u8 name[32];
          __u8 entity[16];
          __u8 selector;
  
          __u8 size;
          __u16 offset;
          __u32 v4l2_type;
          __u32 data_type;
  
          struct uvc_menu_info __user *menu_info;
          __u32 menu_count;
  
          __u32 reserved[4];
  };

extern "C" {

extern int MsSleep(unsigned int t);

#define USE_UVCIOC_CTRL_QUERY  1//add by dfyuan

#if USE_UVCIOC_CTRL_QUERY
#define UVC_XuCmd UVC_XuCmd_V2
extern int UVC_XuCmd_V2(int handle,unsigned char* cmd,unsigned short cs,unsigned char len, unsigned int query, unsigned char unit);
#endif

}

#endif
