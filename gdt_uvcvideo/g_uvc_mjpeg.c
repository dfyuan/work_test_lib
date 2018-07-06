#include "g_media.h"
#include "g_uvc_video.h"
#include "g_uvc_mjpeg.h"

static const char *EP_DATAIN_NAME_MJPEG;
static struct video_device *gMJPEGdev = NULL;

static struct uvc_vs_iptinterface_descriptor uvc_vs_iptinf_desc_mjpeg = {
	.bLength			 = sizeof(uvc_vs_iptinf_desc_mjpeg),
	.bDescriptorType	 = CS_INTERFACE,
	.bDescriptorSubtype  = VS_INPUT_HEADER,
	.bNumFormats		 = 0x02,
	.wTotalLength		 = 0,
	.bmInfo 			 = 0x00,
	.bTerminalLink		 = UVC_TERM_OTT_ID_MJPEG,
	.bStillCaptureMethod = 0x0,
	.bTriggerSupport	 = 0x0,
	.bTriggerUsage		 = 0x0,
	.bControlSize		 = 0x01,
	.bmaControls[0]		 = (1 << 2),
	.bmaControls[1]		 = 0,
};

static const struct usb_interface_descriptor uvc_intf_stream_zero_desc_mjpeg = {
	.bLength			= sizeof(uvc_intf_stream_zero_desc_mjpeg),
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= MJPEG_INTFACE_STREAM,
	.bAlternateSetting	= 0x0,
	.bNumEndpoints		= 0x1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass = SC_VIDEOSTREAMING,
	.bInterfaceProtocol = PC_PROTOCOL_UNDEFINED,
	.iInterface 		= STRING_MJPEG_INTF_STR,
};

static const struct usb_interface_descriptor uvc_intf_stream_desc_mjpeg = {
	.bLength			= sizeof(uvc_intf_stream_desc_mjpeg),
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= MJPEG_INTFACE_STREAM,
	.bAlternateSetting	= 0x01,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass = SC_VIDEOSTREAMING,
	.bInterfaceProtocol = PC_PROTOCOL_UNDEFINED,
	.iInterface 		= STRING_MJPEG_INTF_STR,
};

static struct usb_endpoint_descriptor uvc_dataep_desc_mjpeg = {
	.bLength		  	= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType  	= USB_DT_ENDPOINT,
	.bEndpointAddress 	= USB_DIR_IN,
	.bmAttributes	  	= USB_ENDPOINT_XFER_BULK,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_comp = {
	.bLength			= sizeof(uvc_ss_streaming_comp),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst 			= 14,
	.bmAttributes 		= 0x00,
	.wBytesPerInterval	= 0x0000,
};

struct uvc_vs_mjpeg_format_descriptor uvc_vs_mjpegfmt_desc = {
	.bLength 				= sizeof(uvc_vs_mjpegfmt_desc),
	.bDescriptorType		= CS_INTERFACE,
	.bDescriptorSubtype 	= VS_FORMAT_MJPEG,
	.bFormatIndex			= 0x02,
	.bNumFrameDescriptors	= 0x01,
	.bmFlags				= 0x01,
	.bDefaultFrameIndex		= 0x01,
	.bAspectRatioX			= 0x00,
	.bAspectRatioY			= 0x00,
	.bmInterlaceFlags		= 0x00,
	.bCopyProtect			= 0x00,
};

static struct uvc_vs_mjpeg_frame_descriptor uvc_vs_mjpegfrm_desc[1] = {
{
	.bLength 				= sizeof(uvc_vs_mjpegfrm_desc[0]),
	.bDescriptorType		= CS_INTERFACE,
	.bDescriptorSubtype 	= VS_FRAME_MJPEG,
	.bFrameIndex			= 0x01,
	.bmCapabilities			= 0x00,
	.wWidth					= __constant_cpu_to_le16(640),
	.wHeight				= __constant_cpu_to_le16(480),
	.dwMinBitRate			= __constant_cpu_to_le32(MIN_BITTARTE),
	.dwMaxBitRate			= __constant_cpu_to_le32(MAX_BITTARTE),
	.dwMaxVideoFrameBufferSize = __constant_cpu_to_le32(0x100000),
	.dwDefaultFrameInterval	= __constant_cpu_to_le32(GET_FrameInterval(60)),
	.bFrameIntervalType		= VS_MJPEG_FRAME_INTERVAL_NB,
	.dwFrameInterval[0] 	= __constant_cpu_to_le32(GET_FrameInterval(60)),
	.dwFrameInterval[1] 	= __constant_cpu_to_le32(GET_FrameInterval(30)),
	.dwFrameInterval[2] 	= __constant_cpu_to_le32(GET_FrameInterval(10)),
}
};

static  struct uvc_vs_uncompres_format_descriptor uvc_vs_yuvfmt_des = {
	.bLength                = sizeof(uvc_vs_yuvfmt_des),
	.bDescriptorType        = CS_INTERFACE,
	.bDescriptorSubtype     = VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex           = 0x01,
	.bNumFrameDescriptors   = 0x2,
	.guidFormat             = UVC_GUID_FORMAT_NV12,
	.bBitsPerPixel          = 12,
	.bDefaultFrameIndex     = 1,
	.bAspectRatioX          = 4,
	.bAspectRatioY          = 3,
	.bmInterlaceFlags       = 0,
	.bCopyProtect           = 0,
};

static  struct uvc_vs_uncompres_frame_descriptor uvc_vs_yuvframe_desc[2] = {
{
	.bLength                = sizeof(uvc_vs_yuvframe_desc[0]),
	.bDescriptorType        = CS_INTERFACE,
	.bDescriptorSubtype     = VS_FRAME_UNCOMPRESSED,
	.bFrameIndex            = 1,
	.bmCapabilities         = 0,
	.wWidth                 = __constant_cpu_to_le16(STREAM_WIDTH_640),
	.wHeight                = __constant_cpu_to_le16(STREAM_HIGHT_481),
	.dwMinBitRate           = __constant_cpu_to_le32(18432000),
	.dwMaxBitRate           = __constant_cpu_to_le32(55296000),
	.dwMaxVideoFrameBufferSize      = __constant_cpu_to_le16(0x100000),
	.dwDefaultFrameInterval = __constant_cpu_to_le32(GET_FrameInterval(120)),
	.bFrameIntervalType     = 3,
	.dwFrameInterval[0]     = __constant_cpu_to_le32(GET_FrameInterval(200)),
	.dwFrameInterval[1]     = __constant_cpu_to_le32(GET_FrameInterval(120)),
	.dwFrameInterval[2]     = __constant_cpu_to_le32(GET_FrameInterval(30)),
},
{
	.bLength                = sizeof(uvc_vs_yuvframe_desc[0]),
	.bDescriptorType        = CS_INTERFACE,
	.bDescriptorSubtype     = VS_FRAME_UNCOMPRESSED,
	.bFrameIndex            = 2,
	.bmCapabilities         = 0,
	.wWidth                 = __constant_cpu_to_le16(STREAM_WIDTH_1280),
	.wHeight                = __constant_cpu_to_le16(STREAM_HIGHT_721),
	.dwMinBitRate           = __constant_cpu_to_le32(18432000),
	.dwMaxBitRate           = __constant_cpu_to_le32(55296000),
	.dwMaxVideoFrameBufferSize      = __constant_cpu_to_le16(0x1d0000),
	.dwDefaultFrameInterval = __constant_cpu_to_le32(GET_FrameInterval(30)),
	.bFrameIntervalType     = 3,
	.dwFrameInterval[0]     = __constant_cpu_to_le32(GET_FrameInterval(200)),
	.dwFrameInterval[1]     = __constant_cpu_to_le32(GET_FrameInterval(120)),
	.dwFrameInterval[2]     = __constant_cpu_to_le32(GET_FrameInterval(30)),
},
};

static const struct usb_descriptor_header *mjpeg_function[] = {
	(struct usb_descriptor_header *) &uvc_intf_stream_zero_desc_mjpeg,

	(struct usb_descriptor_header *) &uvc_vs_iptinf_desc_mjpeg,
	(struct usb_descriptor_header *) &uvc_vs_mjpegfmt_desc,
	(struct usb_descriptor_header *) &uvc_vs_mjpegfrm_desc[0],
	(struct usb_descriptor_header *) &uvc_vs_yuvfmt_des,
	(struct usb_descriptor_header *) &uvc_vs_yuvframe_desc[0],
	(struct usb_descriptor_header *) &uvc_vs_yuvframe_desc[1],
	(struct usb_descriptor_header *) &uvc_dataep_desc_mjpeg,
	(struct usb_descriptor_header *) &uvc_ss_streaming_comp,

	NULL,
};

struct usb_endpoint_descriptor *mjpeg_dataep_get(void)
{
	return &uvc_dataep_desc_mjpeg;
}

struct usb_ss_ep_comp_descriptor *mjpeg_dataepss_get(void)
{
	return &uvc_ss_streaming_comp;
}

int mjpeg_stream_info_get(u8 formatId, u8 frameId, u16 *width, u16 *height)
{
	struct uvc_vs_mjpeg_frame_descriptor 	 *framedesc_mjpeg = NULL;
	struct uvc_vs_uncompres_frame_descriptor *framedesc_yuv = NULL;

	if ((frameId > 2) || (frameId == 0))
		return -1;

	if (formatId == 2) {
		framedesc_mjpeg = &uvc_vs_mjpegfrm_desc[frameId - 1];
		*width  		= framedesc_mjpeg->wWidth;
		*height 		= framedesc_mjpeg->wHeight;
	}

	if (formatId == 1) {
		framedesc_yuv = &uvc_vs_yuvframe_desc[frameId - 1];
		*width 		  = framedesc_yuv->wWidth;
		*height       = framedesc_yuv->wHeight;
	}

	return 0;
}

void mjpeg_config_reset(void)
{
	struct video_device *mjpegdev = gMJPEGdev;

	if (mjpegdev->ep_datain) {
		if (mjpegdev->req)
			usb_ep_dequeue(mjpegdev->ep_datain, mjpegdev->req);

		if (mjpegdev->req)
			usb_ep_free_request(mjpegdev->ep_datain, mjpegdev->req);

		mjpegdev->req = NULL;

		mjpegdev->sstatus = STREAME_STOP;
		usb_ep_disable(mjpegdev->ep_datain);
		mjpegdev->ep_datain = NULL;
	}
}

int get_mjpeg_stream_status(void)
{
	struct video_device *mjpegdev = gMJPEGdev;
	return mjpegdev->sstatus;
}

int set_mjpeg_stream_status(enum stream_status status)
{
	struct video_device *mjpegdev = gMJPEGdev;
	mjpegdev->sstatus = status;
	return 0;
}

int mjpeg_stream_start(struct usb_gadget *gadget, gfp_t gfp_flagsc)
{
	int retval = -1;
	struct usb_ep *ep;
	struct video_device *mjpegdev = gMJPEGdev;
	struct usb_endpoint_descriptor	*d;
	struct usb_ss_ep_comp_descriptor *comp_desc;

	gadget_for_each_ep(ep, gadget) {
		if (strcmp(ep->name, EP_DATAIN_NAME_MJPEG) == 0) {
			d = mjpeg_dataep_get();
			comp_desc = mjpeg_dataepss_get();

			ep->desc = d;
			ep->comp_desc = comp_desc;

			ep->mult = 0;
			ep->maxburst = 15;
			retval = usb_ep_enable(ep);
			if (retval == 0) {
				ep->driver_data = mjpegdev;
				mjpegdev->ep_datain = ep;
				printk("##########ep-maxpacket=%d \n", ep->maxpacket);
				printk("##########ep-mult=%d \n", ep->mult);
				printk("##########ep-maxburst=%d \n", ep->maxburst);
				printk("############ SuperSpeed usb_ep_enable #################\n");
				break;
			}
		} else
			continue;

		printk("can't enable %s, retval %d\n", ep->name, retval);
		break;
	}

	if (retval == 0) {
		mjpegdev->sstatus = STREAME_XFER;
		retval = video_stream_xfer_start(mjpegdev);
	}

	printk("start Endpoint %s\n", EP_DATAIN_NAME_MJPEG);

	return retval;
}

static int mjpeg_open(struct inode *inode, struct file *filp)
{
	int retval = 0;
	filp->private_data = gMJPEGdev;
	return retval;
}

static int mjpeg_release(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;
	return 0;
}

static struct file_operations mjpeg_fops = {
	.owner 	 = THIS_MODULE,
	.open  	 = mjpeg_open,
	.release = mjpeg_release,
};

int mjpeg_init(struct usb_gadget *gadget, unsigned long addr_offset)
{
	struct video_device *mjpegdev = NULL;
	struct usb_ep *ep = NULL;
	int retval = 0;

	mjpegdev = kzalloc(sizeof(*mjpegdev), GFP_KERNEL);
	if (mjpegdev == NULL) {
		printk("ERROR: UVC stream interface memory malloc failed \n");
		return -ENOMEM;
	}

	gMJPEGdev = mjpegdev;

	spin_lock_init(&mjpegdev->streamlock);

	ep = usb_ep_autoconfig_ss(gadget, &uvc_dataep_desc_mjpeg, &uvc_ss_streaming_comp);
	if (!ep) {
		printk("can't autoconfigure on %s\n", gadget->name);
		return -ENODEV;
	}

	ep->driver_data 	 = ep;
	EP_DATAIN_NAME_MJPEG = ep->name;
	uvc_dataep_desc_mjpeg.wMaxPacketSize 	  = __constant_cpu_to_le16(MJPEG_DATA_URB_SIZE);
	uvc_vs_iptinf_desc_mjpeg.bEndpointAddress = uvc_dataep_desc_mjpeg.bEndpointAddress;
	uvc_vs_iptinf_desc_mjpeg.wTotalLength 	  = cpu_to_le16(media_total_compute(&mjpeg_function[1], 7));//the number of video stream function

	media_fun_put(mjpeg_function);

	mjpegdev->gadget = gadget;
	mjpegdev->packetlen = __constant_cpu_to_le16(MAX_PAYLOAD_LEN);
	mjpegdev->addr_offset = addr_offset;
	retval = ezybuf_queue_init(&mjpegdev->buf_queue, &mjpeg_fops, sizeof(struct ezybuf_buffer), 0, MJPEG_DRIVER_NAME, mjpegdev);
	if (retval < 0) {
		printk("drvbuf_queue_init fail %d\n", __LINE__);
		goto done;
	}

	printk("DATAIN-%s\n", EP_DATAIN_NAME_MJPEG);

	return 0;

done:
	if (mjpegdev)
		kfree(mjpegdev);
	return -1;
}

void mjpeg_uninit(void)
{
	struct video_device *mjpegdev = gMJPEGdev;

	ezybuf_queue_cancel(&mjpegdev->buf_queue);

	if (gMJPEGdev)
		kfree(gMJPEGdev);

	gMJPEGdev = NULL;
	printk("uvc stream funciotn uninit finished \n");
}

