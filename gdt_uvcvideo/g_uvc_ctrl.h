#ifndef __G_UVC_CTRL_H__
#define __G_UVC_CTRL_H__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/utsname.h>
#include <linux/device.h>
#include <linux/moduleparam.h>
#include <linux/vmalloc.h>
#include <linux/pagemap.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/switch_to.h>
#include <asm/unaligned.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include "../ezy_buf/ezy_buf.h"

#define	CTRL_DRIVER_NAME			"g_uvc_ctrl"
#define	CTRL_IOC_BASE				'C'
#define	CTRL_IOC_MAXNR				2
#define	CTRL_SET_CMD				_IOW(CTRL_IOC_BASE,  1, struct imu_data *)
#define	CTRL_GET_CMD				_IOR(CTRL_IOC_BASE,  2, struct uvc_cmd *)

#define DEVICE_CMD_UNKNOWN			0x00
#define DEVICE_PW_SV				0x01
#define DEVICE_PW_ON				0x02
#define DEVICE_VIDEO_MODE			0x03
#define DEVICE_DISK_MODE			0x04
#define DEVICE_BURN_MODE			0x05
#define DEVICE_FRAME_INFO			0x06


#define OBJ_LEN_MIN					0x00
#define OBJ_LEN_MAX					0x2FD
#define OCU_FOCUS_LEN				0xff

typedef void (*cmd_callback)(void *pdata_old, void *pdata_new);

struct uvc_data {
	u8 *dst;
	u32 size;
	u32 flags;
	cmd_callback callback;
};

struct uvc_frame_info {
	u16 FormatID;			/* uvc driver Format descriptor id */
	u16 FrameID;			/* uvc driver frame descriptor id */
	u16 CompQuality;		/* shoude map to bit rate */
	u32 FrameInterval;		/* shoude map to frame rate */
	u16 Width;				/* actually codec width */
	u16 Height;				/* actually codec Height */
};

struct uvc_cmd {
	unsigned int cmd;
	unsigned int data;
	struct uvc_frame_info frame_info;
};

struct uvccmd_buffer {
	struct drvbuf_buffer dbuf;
	struct uvc_cmd uvccmd;
};

struct unit_control_info {
	struct list_head list;
	__u8 id;
	__u8 cs;
	__u8 flags;
	__u8 info;
	__u16 cur[4];
	__u16 min[4];
	__u16 max[4];
	__u16 def[4];
	__u32 res;
	__u16 len;

	int (*unit_uniq_ctl)(struct unit_control_info *info, u8 req, u8 *data);
	cmd_callback callback;
} __attribute__((packed));

struct unit_control {
	struct list_head controls;
	struct mutex mutex;
};

struct uvc_ctrl_device {
	struct unit_control unit_ctl;
	struct drvbuf_queue vd_queue;//for uvc cmd
	struct ezybuf_queue ctrl_queue;//for io control
	spinlock_t  imulock;
};

struct imu_data {
	float acl_x;
	float acl_y;
	float acl_z;
	float gyro_x;
	float gyro_y;
	float gyro_z;
	long timestamp;
};

#define IMU_SIZE   sizeof(struct imu_data)

void uvc_ctrl_uninit(void);
int uvc_ctrl_init(struct usb_gadget *gadget);
int uvc_media_cmd_put(u32 cmd, u32 data);
int uvc_class_cmd(const struct usb_ctrlrequest *ctrl);

#endif
