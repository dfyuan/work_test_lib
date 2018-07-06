#ifndef __G_MEDIA_H__
#define __G_MEDIA_H__

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
#include "g_uvc_video.h"
#include "g_uvc_ctrl.h"
#include "sysconfig.h"


#define GDT_MEDIA_DEV_NAME		"g_media"
#define	MEDIA_IOC_BASE			'M'
#define	MEDIA_IOC_MAXNR			2

#define UVC_INTFACE_STATUS		0
#define MJPEG_INTFACE_STREAM	1

#define UVC_INTFACE_NUM			(MJPEG_INTFACE_STREAM + 1)
#define MEDIA_INTFACE_NUM		(UVC_INTFACE_NUM)

#define STRING_MANUFACTURER		1
#define STRING_PRODUCT			2
#define STRING_SERIAL			3
#define STRING_UVC_IAD			4
#define STRING_MJPEG_INTF_STR	8

struct media_function {
	struct list_head list;
	const struct usb_descriptor_header **desfun;
};

struct media_device {
	u8 config;
	spinlock_t lock;
	struct class *class;
	struct class_device *class_dev;
	struct usb_gadget  *gadget;
	struct usb_request *req_ctrl;/* for control endpoint */
	struct list_head fun_list;
};

struct media_device *media_get_dev(void);

int media_fun_put(const struct usb_descriptor_header **desfun);
int media_total_compute(const struct usb_descriptor_header **desfun, int count);

#endif

