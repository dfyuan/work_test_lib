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

#include "g_media.h"
#include "g_uvc_video.h"
#include "g_uvc_ctrl.h"
#include "g_uvc_mjpeg.h"


struct wait_param {
	struct work_struct my_work;
	struct usb_ep *ep;
	struct usb_request *req;
};

struct wait_param  epwork;

static struct usb_interface_assoc_descriptor uvc_iad_desc = {
	.bLength 			= sizeof(uvc_iad_desc),
	.bDescriptorType	= USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface	= 0x00,
	.bInterfaceCount	= UVC_INTFACE_NUM,
	.bFunctionClass		= USB_CLASS_VIDEO,
	.bFunctionSubClass	= SC_VIDEO_INTERFACE_COLLECTION,
	.bFunctionProtocol	= PC_PROTOCOL_UNDEFINED,
	.iFunction 			= STRING_UVC_IAD,
};

static struct uvc_vc_interface_descriptor uvc_vc_intf_desc = {
	.bLength 			= sizeof(uvc_vc_intf_desc),
	.bDescriptorType	= CS_INTERFACE,
	.bDescriptorSubtype = VC_HEADER,
	.bcdUVC 			= __constant_cpu_to_le16(UVC_VERSION),
	.wTotalLength		= 0,
	.dwClockFrequency	=  __constant_cpu_to_le32(CLOCK_FREQ),
	.bInCollection		= VC_INTERFACE_COLLECTION_NB,
	.baInterfaceNr[0]	= MJPEG_INTFACE_STREAM,
};

static struct uvc_vc_ittc_descriptor uvc_vc_ittc_desc = {
	.bLength 			= sizeof(uvc_vc_ittc_desc),
	.bDescriptorType	= CS_INTERFACE,
	.bDescriptorSubtype = VC_INPUT_TERMINAL,
	.bTerminalID		= UVC_TERM_CAM_ID,
	.wTerminalType		= __constant_cpu_to_le16(ITT_CAMERA),
	.bAssocTerminal		= 0x00,
	.iTerminal			= 0x00,
	.wObjectiveFocalLengthMin = __constant_cpu_to_le16(OBJ_LEN_MIN),
	.wObjectiveFocalLengthMax = __constant_cpu_to_le16(OBJ_LEN_MAX),
	.wOcularFocalLength	= __constant_cpu_to_le16(OCU_FOCUS_LEN),
	.bControlSize		= VC_ITTC_CONTROL_SIZE,
	.bmControls[0]		= 0x00,
	.bmControls[1]		= 0x00,
	.bmControls[2]		= 0x00,
};

static struct uvc_vc_punit_descriptor uvc_vc_punit_desc = {
	.bLength 			= sizeof(uvc_vc_punit_desc),
	.bDescriptorType	= CS_INTERFACE,
	.bDescriptorSubtype = VC_PROCESSING_UNIT,
	.bUnitID			= UVC_UNIT_PUNIT_ID,
	.bSourceID			= UVC_TERM_CAM_ID,
	.wMaxMultiplier		= 0x0000,
	.bControlSize		= VC_PUNIT_CONTROL_SIZE,
	.bmControls[0]		= 0x00,
	.bmControls[1]		= 0x00,
	.iProcessing		= 0x00,
	.bmVideoStandards	= 0x00,
};

static struct uvc_vc_extension_unit_desciptor uvc_vc_extunit_desc = {
	.bLength 			= sizeof(uvc_vc_extunit_desc),
	.bDescriptorType 	= CS_INTERFACE,
	.bDescriptorSubType = VC_EXTENSION_UNIT,
	.bUnitID 			= UVC_UNIT_EXT_ID,
	.guidExtensionCode	= UVC_GUID_UVC_SUNNY_EXT,
	.bNumControls 		= VC_EXT_UNIT_NR_CONTROLS,
	.bNrInPins 			= VC_EXT_UNIT_NR_IN_PINS,
	.baSourceID[0]		= UVC_UNIT_PUNIT_ID,
	.bControlSize 		= VC_EXT_UNIT_CONTROL_SIZE,
	.bmControls[0]		= 0x07,
	.bmControls[1]		= 0x08,
	.iExtension 		= VC_EXT_UNIT_EXTENSION,
};

static struct uvc_vc_ott_descriptor uvc_vc_ott_desc_mjpeg = {
	.bLength 			= sizeof(uvc_vc_ott_desc_mjpeg),
	.bDescriptorType	= CS_INTERFACE,
	.bDescriptorSubtype = VC_OUTPUT_TERMINAL,
	.bTerminalID		= UVC_TERM_OTT_ID_MJPEG,
	.wTerminalType		= __constant_cpu_to_le16(TT_STREAMING),
	.bAssocTerminal		= 0x00,
	.bSourceID			= UVC_UNIT_EXT_ID,
	.iTerminal			= 0x00,
};

static const struct usb_interface_descriptor uvc_intf_status_desc = {
	.bLength 		 	= sizeof(uvc_intf_status_desc),
	.bDescriptorType 	= USB_DT_INTERFACE,
	.bInterfaceNumber 	= UVC_INTFACE_STATUS,
	.bNumEndpoints 	 	= 0,
	.bInterfaceClass 	= USB_CLASS_VIDEO,
	.bInterfaceSubClass = SC_VIDEOCONTROL,
	.bInterfaceProtocol = PC_PROTOCOL_UNDEFINED,
	.iInterface		 	= STRING_UVC_IAD,
};

static const struct usb_descriptor_header *video_function[] = {
	(struct usb_descriptor_header *) &uvc_iad_desc,
	(struct usb_descriptor_header *) &uvc_intf_status_desc,

	(struct usb_descriptor_header *) &uvc_vc_intf_desc,
	(struct usb_descriptor_header *) &uvc_vc_ittc_desc,
	(struct usb_descriptor_header *) &uvc_vc_punit_desc,
	(struct usb_descriptor_header *) &uvc_vc_extunit_desc,
	(struct usb_descriptor_header *) &uvc_vc_ott_desc_mjpeg,

	NULL,
};

void wait_agent_fn(struct work_struct *data)
{
	struct wait_param * pwait;
	struct video_device *streamdev = NULL;

	if (data == NULL) {
		printk("error no valid param \n");
		return;
	}

	pwait = (struct wait_param *)data;
	if (pwait->ep && pwait->req) {
		streamdev = (struct video_device *)pwait->ep->driver_data;
		if (streamdev->sstatus == STREAME_XFER) {
			video_stream_xfer_complete(pwait->ep, pwait->req);
		}
	}
}

void video_stream_xfer_complete(struct usb_ep *ep, struct usb_request *req)
{
	int ebstatus = 0;
	int  retval = req->status;
	struct video_buffer *videobuf = req->context;
	struct video_device *videodev = ep->driver_data;
	struct ezybuf_buffer *ebuf = NULL;
	struct uvc_stream_header *uvchd = NULL;
	u16 packetlen = videodev->packetlen;
	int bytesused = 0;
	u32 cur_frame;
	u32 byteslen = 0;
	u8 uvc_fid = 0;
	struct timeval scr_time;
	unsigned long flags;
	switch (retval) {
	case 0:
		break;
	case -EOVERFLOW:

	case -EREMOTEIO:
		break;
	case -ECONNABORTED:
	case -ECONNRESET:
	case -ESHUTDOWN:
		if (videobuf->ebuf) {
			ezybuf_wakeup(videobuf->ebuf, EBUF_DONE);
		}

		videodev->sstatus=STREAME_STOP;

		videobuf->ebuf = NULL;
		printk("%s gone (%d), %d/%d\n", ep->name, retval, req->actual, req->length);
		return;
	default:
		break;
	}

	if(videodev->sstatus == STREAME_STOP)
		return ;

	if (videobuf->done) {
		if (videobuf->ebuf) {
			ezybuf_wakeup(videobuf->ebuf, EBUF_DONE);
		}
		ebuf = ezybuf_dqbuf_user(&videodev->buf_queue);
		if (ebuf == NULL) {
			ebstatus = -1;
		} else if (!ebuf->bytesused) {
			if (ebuf) {
				ezybuf_wakeup(ebuf, EBUF_DONE);
			}

			ebstatus = -2;
		}

		if (ebstatus) {
			epwork.ep =  ep;
			epwork.req = req;
			//printk("schedule_work ebstatus=%d \n",ebstatus);
			schedule_work(&(epwork.my_work));
			return;
		}

		videobuf->ebuf = ebuf;
		if (ebuf->bytesused <= (packetlen - UVC_HLE)) {
			videobuf->done = 1;
			byteslen  = ebuf->bytesused + UVC_HLE;
			bytesused = ebuf->bytesused;
		} else {
			videobuf->done = 0;
			byteslen  = packetlen;
			bytesused = packetlen - UVC_HLE;
		}

		videobuf->vaddr = ebuf->vaddr + ebuf->headlen - UVC_HLE;
		videobuf->offset = ebuf->phyaddr + ebuf->headlen - UVC_HLE;

		uvchd = (struct uvc_stream_header *)videobuf->vaddr;
		videobuf->uvc_fid ^= UVC_FID;
		uvchd->bmHeaderInfo = videobuf->uvc_fid | UVC_SCR | UVC_PTS | UVC_EOH;

		do_gettimeofday(&scr_time);
		cur_frame = (u64) usb_gadget_frame_number(videodev->gadget);

		uvchd->dwPresentationTime = scr_time.tv_usec;
		uvchd->scrSourceClock = (((u64)cur_frame & 0x7ff) << 32)  | ((scr_time.tv_sec * 1000000) + scr_time.tv_usec);

		uvchd->bHeaderLength = UVC_HLE;
		videobuf->bytesremain = ebuf->bytesused - bytesused;

	} else {
		bytesused =  packetlen - UVC_HLE;
		if ((videobuf->bytesremain - bytesused) > 0) {
			byteslen  = packetlen;
			videobuf->bytesremain -= bytesused;
		} else {
			byteslen  = videobuf->bytesremain + UVC_HLE;
			bytesused = videobuf->bytesremain;

			videobuf->done = 1;
			uvc_fid = UVC_EOF;
		}

		uvchd = (struct uvc_stream_header *)videobuf->vaddr;
		uvchd->bmHeaderInfo = videobuf->uvc_fid | uvc_fid | UVC_EOH;
		uvchd->bHeaderLength = UVC_HLE;
	}


	if (req != NULL) {
		req->buf = (void *)videobuf->vaddr;
		req->dma = videobuf->offset;
		req->length = byteslen;
	}

	if (videobuf == NULL) {
		printk("videobuf==NULL xxxxxxxxxxxxxx\n");
		return;
	}

	if (videobuf != NULL) {
		videobuf->vaddr  += bytesused;
		videobuf->offset += bytesused;
	}

	if (videodev->ep_datain == NULL) {
		printk("ep_datain==NULL xxxxxxxxxxxxx\n");
		return;
	}

	retval = usb_ep_queue(videodev->ep_datain, req, GFP_ATOMIC);
	if (retval != 0) {
		printk("usb_ep_queue tranfer video data error= %d!!!!\n", retval);
	}

}

int video_stream_xfer_start(struct video_device *videodev)
{
	int	retval = 0;
	struct usb_request	*req;
	struct video_buffer *videobuf = NULL;
	struct uvc_stream_header *vshead;

	videobuf = &videodev->videobuf;
	videobuf->done = 1;
	vshead = &videobuf->vshead;
	vshead->bmHeaderInfo =  UVC_EOH;
	vshead->bHeaderLength = 2;

	if (videodev->req == NULL) {
		videodev->req = usb_ep_alloc_request(videodev->ep_datain, GFP_ATOMIC);
		if (videodev->req == NULL)
			return -ENOMEM;
	}

	req = videodev->req;
	req->buf = (void *)vshead;
	req->length = 2;
	req->complete = video_stream_xfer_complete;
	req->context = videobuf;
	retval = usb_ep_queue(videodev->ep_datain, req, GFP_ATOMIC);
	if (retval != 0) {
		printk("usb_ep_queue error\n");
	}

	return retval;

}


int video_init(struct usb_gadget *gadget)
{
	int retval;

	uvc_vc_intf_desc.wTotalLength = cpu_to_le16(media_total_compute(&video_function[2], 5));//5 is the number of functions,start of video_funciotn[2]

	media_fun_put(video_function);

	//uvc class protocal video control interface
	retval = uvc_ctrl_init(gadget);
	if (retval < 0) {
		printk("uvc_ctrl_init function init failed\n");
		goto done;
	}

	//uvc class protocal video stream interface
	retval = mjpeg_init(gadget, 0);
	if (retval < 0) {
		printk("uvc video fuction init failed \n");
		goto ctrl_done;
	}

	INIT_WORK(&(epwork.my_work), wait_agent_fn);

	return 0;

ctrl_done:
	uvc_ctrl_uninit();
done:
	return -1;
}

void video_uninit(void)
{
	mjpeg_uninit();
	uvc_ctrl_uninit();
}

