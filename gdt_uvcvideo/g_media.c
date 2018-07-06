#include "g_media.h"
#include "gadget_chips.h"
#include "g_uvc_mjpeg.h"
#include "g_uvc_ctrl.h"
#include "g_uvc_video.h"


#define GDT_DRIVER_VERSION  "V3.5.0.0" // 3--usb3.0  5.0.0 version
#define IAD_UVC_NAME	    "Sunny HD WebCam-V3.1.0" //usb3.0 versoin 1.0
#define DEVICE_NAME 		"SUNNY UVC CAMERA"
#define DMA_ADDR_INVALID    (~(dma_addr_t)0)
#define CAMERA_IDVENDOR		0xAAAA
#define CAMERA_IDPRODUCT	0x6666
#define MEDIA_DEV_NUM		1
#define	MEDIA_CONFIG_NUM	1
#define MJPEG_STATUS_BIT	0x02
#define USB_BUF_CTRL_SIZE	1024

static int major;
struct media_device *gMedia = NULL;
static const char shortname[] 	   	  = IAD_UVC_NAME;
static const char product_name[]   	  = IAD_UVC_NAME;
static const char serial[] 	   		  = IAD_UVC_NAME;
static const char iad_uvc_name[]   	  = IAD_UVC_NAME;
static const char manufacturer[] 	  = "Ningbo Sunny Opto Co.,Ltd";
static const char intf_mjpeg_stream[] = "MJPEG Stream Interface";

static struct usb_string webcam_strings[] = {
	{ STRING_MANUFACTURER, 		manufacturer, },
	{ STRING_PRODUCT, 			product_name, },
	{ STRING_SERIAL, 			serial, },
	{ STRING_UVC_IAD, 			iad_uvc_name, },
	{ STRING_MJPEG_INTF_STR,  	intf_mjpeg_stream, },
	{  }
};

static struct usb_gadget_strings webcam_stringtab = {
	.language	= 0x0409,
	.strings	= webcam_strings,
};

static struct usb_device_descriptor media_device_desc = {
	.bLength 		 = sizeof(media_device_desc),
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB 		 = __constant_cpu_to_le16(0x0300),
	.bDeviceClass    = USB_CLASS_MISC,
	.bDeviceSubClass = 0x02,
	.bDeviceProtocol = 0x01,
	.iManufacturer   = STRING_MANUFACTURER,
	.iProduct 		 = STRING_PRODUCT,
	.iSerialNumber   = STRING_SERIAL,
	.bMaxPacketSize0 = 0x09,
	.bNumConfigurations = MEDIA_DEV_NUM,
};

static struct usb_qualifier_descriptor media_dev_qualifier = {
	.bLength 		 = sizeof(media_dev_qualifier),
	.bDescriptorType = USB_DT_DEVICE_QUALIFIER,
	.bcdUSB 		 = __constant_cpu_to_le16(0x0210),
	.bDeviceClass	 = USB_CLASS_PER_INTERFACE,
	.bDeviceSubClass = 0x02,
	.bDeviceProtocol = 0x00,
	.bMaxPacketSize0 = 0x40,
	.bNumConfigurations = 1,
};

static struct usb_config_descriptor media_config_desc = {
	.bLength 			 = sizeof(media_config_desc),
	.bDescriptorType 	 = USB_DT_CONFIG,
	.bNumInterfaces 	 = MEDIA_INTFACE_NUM,
	.bConfigurationValue = MEDIA_CONFIG_NUM,
	.iConfiguration 	 = STRING_PRODUCT,
	.bmAttributes		 = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower			 = 0x40,
};

static struct usb_bos_descriptor media_bos_desc = {
	.bLength 			= sizeof(media_bos_desc),
	.bDescriptorType 	= USB_DT_BOS,
	.wTotalLength 		= 0,
	.bNumDeviceCaps 	= 0x2,
};

static struct usb_ext_cap_descriptor media_ext_desc = {
	.bLength 			= sizeof(media_ext_desc),
	.bDescriptorType 	= USB_DT_DEVICE_CAPABILITY,
	.bDevCapabilityType = USB_CAP_TYPE_EXT,
	.bmAttributes 		= cpu_to_le32(USB_LPM_SUPPORT | USB_BESL_SUPPORT),
};

static struct usb_ss_cap_descriptor media_sscap_desc = {
	.bLength 			= sizeof(media_sscap_desc),
	.bDescriptorType 	= USB_DT_DEVICE_CAPABILITY,
	.bDevCapabilityType = USB_SS_CAP_TYPE,
	.bmAttributes 		= 0,
	.wSpeedSupported 	= cpu_to_le16(USB_HIGH_SPEED_OPERATION | USB_5GBPS_OPERATION),
	.bFunctionalitySupport = 0,
	.bU1devExitLat 		= 0,
	.bU2DevExitLat 		= 0,
};

int media_total_compute(const struct usb_descriptor_header **desfun, int count)
{
	int i;
	int total = 0;
	const struct usb_descriptor_header *fun;

	for (i = 0; i < count; i++) {
		fun = desfun[i];
		total += fun->bLength;
	}

	return total;
}

int media_fun_put(const struct usb_descriptor_header **desfun)
{
	unsigned long flags;
	struct media_function *mdfun;
	struct media_device *mdev = gMedia;

	mdfun = kzalloc(sizeof(*mdfun), GFP_KERNEL);
	if (mdfun == NULL)
		return -1;

	INIT_LIST_HEAD(&mdfun->list);
	mdfun->desfun = desfun;
	spin_lock_irqsave(&mdev->lock, flags);
	list_add_tail(&mdfun->list, &mdev->fun_list);
	spin_unlock_irqrestore(&mdev->lock, flags);

	return 0;
}

static void media_fun_free(void)
{
	unsigned long flags;
	struct media_function *mdfun;
	struct list_head *list;
	struct list_head *tmp;;
	struct media_device *mdev = gMedia;

	spin_lock_irqsave(&mdev->lock, flags);
	list_for_each_safe(list, tmp, &mdev->fun_list) {
		mdfun = list_entry(list, struct media_function, list);
		list_del(&mdfun->list);
		kfree(mdfun);
	}
	spin_unlock_irqrestore(&mdev->lock, flags);
	return ;
}

static int media_buf_config(struct usb_gadget *gadget, u8 *buf, u8 type, unsigned index)
{
	int len = USB_DT_CONFIG_SIZE;
	struct list_head *list;
	struct media_function *mdfun;
	const struct usb_descriptor_header **desfun = NULL;
	struct usb_config_descriptor *cp = (struct usb_config_descriptor *)buf;
	struct media_device *mdev = get_gadget_data(gadget);

	if (index > MEDIA_CONFIG_NUM)
		return -EINVAL;

	*cp = media_config_desc;

	list_for_each(list, &mdev->fun_list) {
		mdfun = list_entry(list, struct media_function, list);
		desfun = mdfun->desfun;
		len += usb_descriptor_fillbuf(len + (u8*)buf, USB_BUF_CTRL_SIZE - len, desfun);
		if (len < 0)
			return len;
	}

	if (len > 0xffff)
		return -EINVAL;

	cp->bLength = USB_DT_CONFIG_SIZE;
	cp->bDescriptorType = USB_DT_CONFIG;
	cp->wTotalLength = cpu_to_le16(len);
	cp->bmAttributes |= USB_CONFIG_ATT_ONE;

	((struct usb_config_descriptor *) buf)->bDescriptorType = type;

	return len;
}

struct media_device *media_get_dev(void)
{
	return gMedia;
}

void media_epreq_free(struct usb_ep *ep, struct usb_request *req)
{
	if (req->buf) {
		kfree(req->buf);
	}

	usb_ep_free_request(ep, req);
}

static void media_requst_free(struct usb_gadget *gadget)
{
	struct media_device *dev = get_gadget_data(gadget);
	if (dev->req_ctrl) {
		dev->req_ctrl->length = USB_BUF_CTRL_SIZE;
		media_epreq_free(gadget->ep0, dev->req_ctrl);
	}
	printk("media_requst_free ep0 free \n");
}

static void gadget_config_reset(struct media_device *dev)
{
	if (dev->config == 0)
		return;

	printk("gadget_config_reset\n");
	mjpeg_config_reset();
	dev->config = 0;
}

void gadget_setup_complete(struct usb_ep *ep, struct usb_request *req)
{
	if (req->status || req->actual != req->length) {
		printk("gadget_setup_complete--> %d,%d/%d\n", req->status, req->actual, req->length);
	}
}

static int usb_gadget_get_bos_desc(u8 *buf)
{
	struct usb_ext_cap_descriptor	*usb_ext;
	struct usb_ss_cap_descriptor	*ss_cap;
	struct usb_bos_descriptor	*bos = buf;

	bos->bLength = USB_DT_BOS_SIZE;
	bos->bDescriptorType = USB_DT_BOS;

	bos->wTotalLength = cpu_to_le16(USB_DT_BOS_SIZE);
	bos->bNumDeviceCaps = 0;

	usb_ext = buf + le16_to_cpu(bos->wTotalLength);
	bos->bNumDeviceCaps++;
	le16_add_cpu(&bos->wTotalLength, USB_DT_USB_EXT_CAP_SIZE);
	usb_ext->bLength = USB_DT_USB_EXT_CAP_SIZE;
	usb_ext->bDescriptorType = USB_DT_DEVICE_CAPABILITY;
	usb_ext->bDevCapabilityType = USB_CAP_TYPE_EXT;
	usb_ext->bmAttributes = cpu_to_le32(USB_LPM_SUPPORT | USB_BESL_SUPPORT);


	ss_cap = buf + le16_to_cpu(bos->wTotalLength);
	bos->bNumDeviceCaps++;
	le16_add_cpu(&bos->wTotalLength, USB_DT_USB_SS_CAP_SIZE);
	ss_cap->bLength = USB_DT_USB_SS_CAP_SIZE;
	ss_cap->bDescriptorType = USB_DT_DEVICE_CAPABILITY;
	ss_cap->bDevCapabilityType = USB_SS_CAP_TYPE;
	ss_cap->bmAttributes = 0;
	ss_cap->wSpeedSupported = cpu_to_le16(USB_HIGH_SPEED_OPERATION | USB_5GBPS_OPERATION);
	ss_cap->bFunctionalitySupport = USB_LOW_SPEED_OPERATION;

	ss_cap->bU1devExitLat = USB_DEFAULT_U1_DEV_EXIT_LAT;
	ss_cap->bU2DevExitLat = cpu_to_le16(USB_DEFAULT_U2_DEV_EXIT_LAT);
	return le16_to_cpu(bos->wTotalLength);
}

static int media_stand_setup(struct usb_gadget *gadget, const struct usb_ctrlrequest *ctrl)
{
	struct media_device *dev = get_gadget_data(gadget);
	struct usb_request	*req = dev->req_ctrl;
	int retval = -EOPNOTSUPP;
	u16 w_index = le16_to_cpu(ctrl->wIndex);
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16 w_length = le16_to_cpu(ctrl->wLength);

	//printk("media_stand_setup req%02x.%02x v%04x i%04x l%d\n",ctrl->bRequestType, ctrl->bRequest,w_value, w_index, w_length);
	switch (ctrl->bRequest) {
	case USB_REQ_GET_DESCRIPTOR:
		if (ctrl->bRequestType != (USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE))
			goto unknown;

		switch (w_value >> 8) {
		case USB_DT_DEVICE:
			if (gadget_is_superspeed(gadget)) {
				if (gadget->speed >= USB_SPEED_SUPER) {
					printk("USB_SPEED_SUPER \n");
					media_device_desc.bcdUSB = cpu_to_le16(0x0300);
					media_device_desc.bMaxPacketSize0 = 0x09;
				} else {
					printk("USB_SPEED_HIGH 0\n");
					media_device_desc.bcdUSB = cpu_to_le16(0x0210);
				}
			} else {
				media_device_desc.bcdUSB = cpu_to_le16(0x0200);
			}

			retval = min(w_length, (u16) sizeof(media_device_desc));
			memcpy(req->buf, &media_device_desc, retval);
			break;

		case USB_DT_DEVICE_QUALIFIER:
			if (!gadget->speed)
				break;
			retval = min(w_length, (u16) sizeof media_dev_qualifier);
			memcpy(req->buf, &media_dev_qualifier, retval);
			break;

		case USB_DT_OTHER_SPEED_CONFIG:
			if (!gadget->speed)
				break;
			goto get_config;

		case USB_DT_CONFIG:
get_config:
			retval = media_buf_config(gadget, req->buf, w_value >> 8, w_value & 0xff);

			if (retval >= 0)
				retval = min(w_length, (u16)retval);
			else
				printk("error in get config desc(%d)....\n", retval);
			break;

		case USB_DT_STRING:
			retval = usb_gadget_get_string(&webcam_stringtab, w_value & 0xff, req->buf);
			if (retval >= 0)
				retval = min(w_length, (u16) retval);
			break;

		case USB_DT_BOS:
			if (gadget_is_superspeed(gadget)) {
				retval = usb_gadget_get_bos_desc(req->buf);
				retval = min(w_length, (u16) retval);
			}
			break;

		default :
			break;
		}
		break;

	case USB_REQ_SET_CONFIGURATION:
		if (ctrl->bRequestType != (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE))
			goto unknown;
		dev->config = w_value;
		printk("Super speed config #%d\n", w_value);

		retval = 0;
		break;

	case USB_REQ_GET_CONFIGURATION:
		if (ctrl->bRequestType != (USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE))
			goto unknown;
		*(u8 *)req->buf = dev->config;
		retval = min(w_length, (u16)1);
		break;

	case USB_REQ_SET_INTERFACE:
		if (ctrl->bRequestType != (USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_INTERFACE))
			goto unknown;

		printk("set interface: %d/%d\n", w_index, w_value);
		retval = 0;
		break;

	case USB_REQ_GET_INTERFACE:
		if (ctrl->bRequestType != (USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_INTERFACE))
			goto unknown;

		if (!dev->config)
			break;
		if (w_index != 0) {
			retval = -EDOM;
			break;
		}
		*(u8 *)req->buf = 0;
		retval = min(w_length, (u16)1);
		break;

	default:
unknown:
		printk("unknown control req%02x.%02x v%04x i%04x l%d\n", ctrl->bRequestType, ctrl->bRequest, w_value, w_index, w_length);
	}

	return retval;
}

static int gadget_setup(struct usb_gadget *gadget, const struct usb_ctrlrequest *ctrl)
{
	struct media_device *dev = get_gadget_data(gadget);
	struct usb_request	*req = dev->req_ctrl;
	int retval = -EOPNOTSUPP;
	u8	bmInterface = le16_to_cpu(ctrl->wIndex) & 0xFF;

	//printk("media_setup req%02x.%02x v%04x i%04x l%d\n",ctrl->bRequestType, ctrl->bRequest,ctrl->wValue, ctrl->wIndex ,ctrl->wLength);
	if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS) {
		if (bmInterface < (UVC_INTFACE_NUM))
			retval = uvc_class_cmd(ctrl);
		else
			printk("not support intf or endpoint %d", bmInterface);
	} else {
		retval = media_stand_setup(gadget, ctrl);
	}

	if (retval >= 0) {
		req->length = retval;
		req->zero = (retval < le16_to_cpu(ctrl->wLength) && ((retval % gadget->ep0->maxpacket)) == 0);
		gadget->speed = USB_SPEED_SUPER;
		gadget->max_speed = USB_SPEED_SUPER;
		retval = usb_ep_queue(gadget->ep0, req, GFP_ATOMIC);
		if (retval < 0) {
			printk("ep_queue --> %d\n", retval);
			req->status = 0;
		}
	}
	return retval;
}


static int gaget_ctrlchannle_prepare(struct media_device *dev)
{
	struct usb_gadget *gadget = dev->gadget;

	dev->req_ctrl = usb_ep_alloc_request(gadget->ep0, GFP_KERNEL);
	if (dev->req_ctrl == NULL) {
		printk("uvc request ep0 failed! \n");
		return -ENOMEM;
	}

	dev->req_ctrl->buf = kmalloc(USB_BUF_CTRL_SIZE, GFP_KERNEL);
	dev->req_ctrl->dma = DMA_ADDR_INVALID;
	if (dev->req_ctrl->buf == NULL) {
		printk("uvc ctrl ep0 request buffer failed! \n");
		usb_ep_free_request(gadget->ep0, dev->req_ctrl);
		dev->req_ctrl = NULL;
		return -ENOMEM;
	}

	dev->req_ctrl->complete = gadget_setup_complete;

	return 0;
}

static void gadget_disconnect(struct usb_gadget *gadget)
{
	struct media_device *dev = get_gadget_data(gadget);
	unsigned long flags;

	printk("gadget_disconnect \n");

	spin_lock_irqsave(&dev->lock, flags);
	gadget_config_reset(dev);
	spin_unlock_irqrestore(&dev->lock, flags);
}

static void gadget_unbind(struct usb_gadget *gadget)
{
	printk("gadget_unbind \n");
	video_uninit();
	media_fun_free();
	media_requst_free(gadget);
	set_gadget_data(gadget, NULL);
}

static int gadget_bind(struct usb_gadget *gadget)
{
	struct media_device *dev = gMedia;
	int retval  = -ENOMEM;

	dev->gadget = gadget;
	set_gadget_data(gadget, dev);
	gadget->ep0->driver_data = dev;

	//setup gadget control channle
	retval = gaget_ctrlchannle_prepare(dev);
	if	(retval < 0) {
		printk("gadget ctrl channle init failed! \n");
		return retval;
	}

	//now set usb ep0 state
	usb_ep_autoconfig_reset(gadget);

	//set gaget data channle
	retval = video_init(gadget);
	if (retval < 0) {
		printk("video_init fail!!\n");
		goto done;
	}

	printk("%s, version: " GDT_DRIVER_VERSION "\n", product_name);

	return 0;

done:
	gadget_unbind(gadget);
	return retval;
}

static void gadget_suspend(struct usb_gadget *gadget)
{
	//TBD
	//printk("gadget_suspend\n");
}

static void gadget_resume(struct usb_gadget *gadget)
{
	//TBD
	printk("gadget_resume\n");
}

static void gadget_delete(struct kref *kref)
{
	//TBD
	printk("gadget_delete\n");
}

static int media_open(struct inode *inode, struct file *filp)
{
	filp->private_data = gMedia;
	return 0;
}

static int media_release(struct inode *inode, struct file *filp)
{
	struct media_device *dev = filp->private_data;
	filp->private_data = NULL;
	return 0;
}

static long media_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	char* parg = (char*)arg;
	int retval = 0;
	struct media_device *dev = filp->private_data;

	switch (cmd) {
	default:
		printk("media_ioctl err\n");
		retval = -1;
		break;
	}

	return retval;
}

static struct file_operations media_fops = {
	.owner 	 		= THIS_MODULE,
	.open  	 		= media_open,
	.release 		= media_release,
	.unlocked_ioctl = media_ioctl,
};

static struct usb_gadget_driver gadget_driver = {
	.max_speed	= USB_SPEED_SUPER,
	.function	= (char *) product_name,
	.bind		= gadget_bind,
	.unbind		= __exit_p(gadget_unbind),

	.setup		= gadget_setup,
	.disconnect	= gadget_disconnect,
	.reset		= gadget_disconnect,
	.suspend	= gadget_suspend,
	.resume		= gadget_resume,

	.driver 	= {
	.name		= (char *) shortname,
	.owner		= THIS_MODULE,
	},
};

static int __init media_init(void)
{
	struct media_device *dev = NULL;
	int retval = 0;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		printk("malloc device memory failed \n");
		retval = -ENOMEM;
		goto done;
	}

	gMedia 		   = dev;
	major 		   = register_chrdev(0, DEVICE_NAME, &media_fops);// 0 for daynamic alloc major number
	dev->class 	   = class_create(THIS_MODULE, DEVICE_NAME);//create node on /sys dir
	dev->class_dev = device_create(dev->class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);//create bide on /dev dir

	spin_lock_init(&dev->lock);
	INIT_LIST_HEAD(&dev->fun_list);

	//now register gadget driver
	usb_gadget_probe_driver(&gadget_driver);

	printk("Init UVC Gagdet Driver ,Built on "__DATE__ " " __TIME__ ".\n");
done:
	return retval;
}

static void __exit media_uninit(void)
{
	struct media_device *dev = gMedia;

	usb_gadget_unregister_driver(&gadget_driver);
	unregister_chrdev(major, DEVICE_NAME);
	device_destroy(dev->class, MKDEV(major, 0));
	class_destroy(dev->class);
	if (dev)
		kfree(dev);

	gMedia = NULL;
	printk("uvc gadget driver uninit finished \n");
}
module_init(media_init);
module_exit(media_uninit);
MODULE_DESCRIPTION("usb gadget driver");
MODULE_AUTHOR("<dfyuan@sunnyoptical.com>");
MODULE_LICENSE("GPL");
