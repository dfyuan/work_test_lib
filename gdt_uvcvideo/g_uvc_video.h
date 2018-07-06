#ifndef __G_UVC_VIDEO_H__
#define __G_UVC_VIDEO_H__

#include "../ezy_buf/ezy_buf.h"
#include "g_uvc.h"

#define UVC_VERSION			(0x0110)
#define CLOCK_FREQ			(0x01C9C380)
enum stream_status {
	STREAME_STOP = 0,
	STREAME_XFER,
};

struct video_buffer {
	u8 uvc_fid;
	struct ezybuf_buffer *ebuf;
	u32 done;
	int bytesremain;
	unsigned long vaddr;
	unsigned long offset;

	unsigned long vaddrremain;
	unsigned int siezremain;
	struct uvc_stream_header vshead;
};

struct video_device {
	struct usb_gadget *gadget;
	struct usb_ep *ep_datain;
	struct usb_request *req;
	u16 packetlen;
	struct ezybuf_queue buf_queue;
	struct video_buffer videobuf;
	unsigned long addr_offset;
	enum stream_status sstatus;
	spinlock_t  streamlock;
};


int video_init(struct usb_gadget *gadget);
void video_uninit(void);

int video_stream_xfer_start(struct video_device *videodev);
void video_stream_xfer_complete(struct usb_ep *ep, struct usb_request *req);



#endif

