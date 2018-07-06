#ifndef __G_UVC_MJPEG_H__
#define __G_UVC_MJPEG_H__

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

#define	MJPEG_DRIVER_NAME		"g_uvc_mjpeg"

#define GET_FrameInterval(x)	(10000000/(x))// unit 100ns
#define MIN_BITTARTE			(0x10000)	// 64K Bytes
#define MAX_BITTARTE			(0xFA0000)	// 16000K Bytes

#define STREAM_WIDTH_640		640
#define STREAM_HIGHT_481		481 //for timestamp

#define STREAM_WIDTH_1280		1280
#define STREAM_HIGHT_721		721

#define MJPEG_DATA_URB_SIZE		0x400
#define MAX_PAYLOAD_LEN 		0x8000
#define STILL_CAP_METHOD		0x00
#define TRIGGER_SUPPORT 		0x00
#define TRIGGER_USAGE			0x00

int mjpeg_init(struct usb_gadget *gadget, unsigned long addr_offset);
void mjpeg_uninit(void);
int mjpeg_stream_start(struct usb_gadget *gadget, gfp_t gfp_flagsc);
void mjpeg_config_reset(void);
int mjpeg_stream_info_get(u8 formatId, u8 frameId, u16 *width, u16 *height);
int get_mjpeg_stream_status(void);
int set_mjpeg_stream_status(enum stream_status status);
#endif
