#include "g_media.h"
#include "g_uvc_ctrl.h"
#include "g_uvc_video.h"
#include "g_uvc_mjpeg.h"
#include "sysconfig.h"

#define SENSOR_MODE0 0x01
#define SENSOR_MODE1 0x02
#define SENSOR_MODE2 0x03


extern struct media_device *gMedia;
static struct imu_data g_imudata;
static int  g_devicemode=SENSOR_MODE0;

static struct uvc_ctrl_device *gUvcCtrldev = NULL;
static struct uvc_video_probecomit_controls uvc_mjpeg_vpcc_cur;
static struct uvc_video_probecomit_controls uvc_mjpeg_vpcc_cmt;

static void uvc_videomode_cmd_put(void *pdata_old, void *pdata_new);
static void uvc_diskmode_put(void *pdata_old, void *pdata_new);
static void uvc_burningmode_put(void *pdata_old, void *pdata_new);
static int uvc_vc_default_ctl(struct unit_control_info *info, u8 req, u8 *data);
static int uvc_unit_find_info(struct unit_control *uctl, struct unit_control_info **info, u8 id, u8 cs);


static struct uvc_video_probecomit_controls uvc_mjpeg_vpcc_def = {
	.bmHint 			= __constant_cpu_to_le16(0x0001),
	.bFormatIndex		= 0x01,
	.bFrameIndex		= 0x01,
	.dwFrameInterval	= __constant_cpu_to_le32(83333),//120fps
	.wKeyFrameRate		= 0x0000,
	.wPFrameRate		= 0x0000,
	.wCompQuality		= 0x0000,
	.wCompWindowSize	= 0x0000,
	.wDelay 			= 0x2, //2ms
	.dwMaxVideoFrameSize = __constant_cpu_to_le32(0x1d0000),
	.dwMaxPayloadTransferSize = __constant_cpu_to_le32(MAX_PAYLOAD_LEN),
	.dwClockFrequency	= __constant_cpu_to_le32(0x1017DF80),
	.bmFramingInfo		= 0x03,
	.bPreferedVersion	= UVC_PREFERRED_VER,
	.bMinVersion		= UVC_MIN_VER,
	.bMaxVersion		= UVC_MAX_VER,
};

static struct uvc_video_probecomit_controls uvc_mjpeg_vpcc_min = {
	.bmHint 			= __constant_cpu_to_le16(0x0001),
	.bFormatIndex		= 0x01,
	.bFrameIndex		= 0x01,
	.dwFrameInterval	= __constant_cpu_to_le32(333333),//30fps
	.wKeyFrameRate		= 0x0000,
	.wPFrameRate		= 0x0000,
	.wCompQuality		= 0x0000,
	.wCompWindowSize	= 0x0000,
	.wDelay 			= 0x2, //2ms
	.dwMaxVideoFrameSize = __constant_cpu_to_le32(0x1d0000),
	.dwMaxPayloadTransferSize = __constant_cpu_to_le32(MAX_PAYLOAD_LEN),
	.dwClockFrequency	= __constant_cpu_to_le32(0x1017DF80),
	.bmFramingInfo		= 0x03,
	.bPreferedVersion	= UVC_PREFERRED_VER,
	.bMinVersion		= UVC_MIN_VER,
	.bMaxVersion		= UVC_MAX_VER,
};

static struct uvc_video_probecomit_controls uvc_mjpeg_vpcc_max = {
	.bmHint 			= __constant_cpu_to_le16(0x0001),
	.bFormatIndex		= 0x01,
	.bFrameIndex		= 0x01,
	.dwFrameInterval	= __constant_cpu_to_le32(50000),//200fps
	.wKeyFrameRate		= 0x0000,
	.wPFrameRate		= 0x0000,
	.wCompQuality		= 0x0000,
	.wCompWindowSize	= 0x0000,
	.wDelay 			= 0x2, //2m
	.dwMaxVideoFrameSize = __constant_cpu_to_le32(0x1d0000),
	.dwMaxPayloadTransferSize = __constant_cpu_to_le32(MAX_PAYLOAD_LEN),
	.dwClockFrequency	= __constant_cpu_to_le32(0x1017DF80),
	.bmFramingInfo		= 0x03,
	.bPreferedVersion	= UVC_PREFERRED_VER,
	.bMinVersion		= UVC_MIN_VER,
	.bMaxVersion		= UVC_MAX_VER,
};

struct unit_control_info uvc_ctrls[] = {
	{
		.id 	= UVC_UNIT_EXT_ID,
		.cs 	= XU_IMU_DATA,
		.flags 	= UVC_CONTROL_GET_RANGE | UVC_CONTROL_SET_CUR,
		.cur[0] = 0x00,
		.cur[1] = 0x01,
		.cur[2] = 0x02,
		.cur[3] = 0x55,
		.min[0] = 0x00,
		.min[1] = 0x00,
		.min[2] = 0x00,
		.max[0] = 0x08,
		.max[1] = 0x08,
		.max[2] = 0x08,
		.max[3] = 0xff,
		.def[0] = 0x00,
		.def[1] = 0x00,
		.def[2] = 0x00,
		.def[3] = 0x00,
		.res  	= 0x02,
		.info 	= UVC_SUPPORT_GET_VALUE | UVC_SUPPORT_SET_VALUE,
		.len  	= 56,//imu data struct importim can't set to 32
		.unit_uniq_ctl = uvc_vc_default_ctl,
	},
	{
		.id 	= UVC_UNIT_EXT_ID,
		.cs 	= XU_VIDEO_MODE,
		.flags 	= UVC_CONTROL_GET_RANGE | UVC_CONTROL_SET_CUR,
		.cur[0] = 0x00,
		.min[0] = 0x00,
		.max[0] = 0x08,
		.def[0] = 0x00,
		.res  	= 0x01,
		.info 	= UVC_SUPPORT_GET_VALUE | UVC_SUPPORT_SET_VALUE,
		.len  	= 0x02,
		.unit_uniq_ctl 	= uvc_vc_default_ctl,
		.callback 		= uvc_videomode_cmd_put,
	},
	{
		.id 	= UVC_UNIT_EXT_ID,
		.cs		= XU_DISK_MODE,
		.flags 	= UVC_CONTROL_GET_RANGE | UVC_CONTROL_SET_CUR,
		.cur[0] = 0x00,
		.min[0] = 0x00,
		.max[0] = 0x08,
		.def[0] = 0x00,
		.res  	= 0x01,
		.info 	= UVC_SUPPORT_GET_VALUE | UVC_SUPPORT_SET_VALUE,
		.len  	= 0x02,
		.unit_uniq_ctl 	= uvc_vc_default_ctl,
		.callback 	   	= uvc_diskmode_put,
	},
	{
		.id 	= UVC_UNIT_EXT_ID,
		.cs 	= XU_BURNING_MODE,
		.flags 	= UVC_CONTROL_GET_RANGE | UVC_CONTROL_SET_CUR,
		.cur[0] = 0x00,
		.min[0] = 0x00,
		.max[0] = 0x08,
		.def[0] = 0x00,
		.res  	= 0x01,
		.info 	= UVC_SUPPORT_GET_VALUE | UVC_SUPPORT_SET_VALUE,
		.len  	= 0x02,
		.unit_uniq_ctl 	= uvc_vc_default_ctl,
		.callback 	  	= uvc_burningmode_put,
	}
};

static void uvc_cmd_putcmd(struct uvccmd_buffer *ub, u32 cmd, u32 data, struct uvc_frame_info *frame_info)
{
	ub->uvccmd.cmd = cmd;
	ub->uvccmd.data = data;
	if (frame_info)
		memcpy(&ub->uvccmd.frame_info, frame_info, sizeof(struct uvc_frame_info));
	else
		memset(&ub->uvccmd.frame_info, 0, sizeof(struct uvc_frame_info));
}

static inline void uvccmd_getcmd(struct uvccmd_buffer *ub, void *arg)
{
	int retval;
	retval = copy_to_user(arg, &ub->uvccmd, sizeof(ub->uvccmd));
}

static int uvc_cmd_put(struct drvbuf_queue *q, u32 cmd, u32 data, struct uvc_frame_info *frame_info)
{
	struct uvccmd_buffer *ub = NULL;
	struct drvbuf_buffer *db = NULL;

	db = drvbuf_dqempty(q);
	if (db == NULL)
		return -EFAULT;

	ub = container_of(db, struct uvccmd_buffer, dbuf);
	uvc_cmd_putcmd(ub, cmd, data, frame_info);
	drvbuf_wakeup(db);

	return 0;
}

static int uvc_cmd_get(struct drvbuf_queue *q, void *arg)
{
	struct uvccmd_buffer *ub = NULL;
	struct drvbuf_buffer *db = NULL;

	db = drvbuf_dqfull(q);
	if (db == NULL)
		return -EFAULT;

	ub = container_of(db, struct uvccmd_buffer, dbuf);
	uvccmd_getcmd(ub, arg);
	drvbuf_qempty(q, db);
	drvbuf_qfull(q, db);

	return 0;
}

int uvc_media_cmd_put(u32 cmd, u32 data)
{
	return uvc_cmd_put(&gUvcCtrldev->vd_queue, cmd, data, NULL);
}

static void uvc_mjpeg_cmd_put(void *pdata_old, void *pdata_new)
{
	int reval;

	struct uvc_ctrl_device *ctrldev = gUvcCtrldev;
	struct uvc_video_probecomit_controls *data_new = pdata_new;
	struct uvc_video_probecomit_controls *data_old = pdata_old;
	struct uvc_frame_info frame_info;


	if (data_new->bFormatIndex != data_old->bFormatIndex || data_new->bFrameIndex != data_old->bFrameIndex) {
		reval = mjpeg_stream_info_get(data_new->bFormatIndex, data_new->bFrameIndex, &frame_info.Width, &frame_info.Height);
		if (reval == 0) {
			frame_info.CompQuality = data_new->wCompQuality;
			frame_info.FrameInterval = data_new->dwFrameInterval;
			frame_info.FrameID = data_new->bFrameIndex;
			frame_info.FormatID = data_new->bFormatIndex;
			uvc_cmd_put(&ctrldev->vd_queue, DEVICE_FRAME_INFO, 0, &frame_info);
		}
	}
}

static void uvc_burningmode_put(void *pdata_old, void *pdata_new)
{
	struct uvc_ctrl_device *ctrldev = gUvcCtrldev;
	printk("uvc_burningmode_put \n");
	uvc_cmd_put(&ctrldev->vd_queue,DEVICE_BURN_MODE,0,NULL);
}
static void uvc_diskmode_put(void *pdata_old, void *pdata_new)
{
	struct uvc_ctrl_device *ctrldev = gUvcCtrldev;
	printk("uvc_diskmode_put \n");
	uvc_cmd_put(&ctrldev->vd_queue, DEVICE_DISK_MODE, 0, NULL);
}

static void uvc_videomode_cmd_put(void *pdata_old, void *pdata_new)
{
	u16 data_new = *(u16 *)pdata_new;

	u8 data_temp;
	struct uvc_ctrl_device *ctrldev = gUvcCtrldev;
	data_temp = (u8)data_new;
	g_devicemode=data_temp;
	//first set device to streamoff state ,then set device video mode ,
	uvc_media_cmd_put(DEVICE_PW_SV, 0);
	mdelay(10);
	printk("uvc_videomode_cmd_put mode=%d \n",data_temp);
	uvc_cmd_put(&ctrldev->vd_queue, DEVICE_VIDEO_MODE, data_temp, NULL);
}

void ep0_ctrl_complete(struct usb_ep *ep, struct usb_request *req)
{
	if (req->status || req->actual != req->length) {
		printk("ep0_ctrl_complete--> %d,%d/%d\n", req->status, req->actual, req->length);
	}
}

static void uvc_data_complete(struct usb_ep *ep, struct usb_request *req)
{
	int retval = 0;
	struct uvc_data *dstdata = NULL;
	struct uvc_video_probecomit_controls *pVpcc;

	if (req->status || req->actual != req->length) {
		printk("setup complete-->%d, %d/%d\n", req->status, req->actual, req->length);
	}

	if (req->actual == req->length) {
		if (req->context) {
			dstdata = req->context;

			if (!dstdata->flags) {
				if (dstdata->callback) {
					dstdata->callback(dstdata->dst, req->buf);
				}

				retval = min(req->actual, dstdata->size);
				memcpy(dstdata->dst, req->buf, retval);
			}
			pVpcc = (struct uvc_video_probecomit_controls *)(req->buf);
		}

		req->complete = ep0_ctrl_complete;
		if (retval == sizeof(uvc_mjpeg_vpcc_cur)) {
			pVpcc = (struct uvc_video_probecomit_controls *)(dstdata->dst);
			pVpcc->dwMaxVideoFrameSize = __constant_cpu_to_le32(0x1d0000);
			pVpcc->dwMaxPayloadTransferSize = __constant_cpu_to_le32(MAX_PAYLOAD_LEN);

			if(g_devicemode==SENSOR_MODE0){
				pVpcc->dwFrameInterval= __constant_cpu_to_le32(83333);
			}
			else if(g_devicemode==SENSOR_MODE1){
				pVpcc->dwFrameInterval= __constant_cpu_to_le32(333333);
			}
			else if(g_devicemode==SENSOR_MODE2){
				pVpcc->dwFrameInterval= __constant_cpu_to_le32(50000);
			}
		}
	} else {
		int i;
		unsigned char *data = req->buf;
		for (i = 0; i < req->actual; i++) {
			printk("req->buf[%d]=0x%x\n", i, data[i]);
		}

		req->complete = ep0_ctrl_complete;
	}
}

static void uvc_data_setup(void *dst, u32 size, cmd_callback callback, u32 flags)
{
	struct usb_request *req = media_get_dev()->req_ctrl;

	static struct uvc_data uvcdata;

	uvcdata.dst  = dst;
	uvcdata.size = size;
	uvcdata.flags = flags;
	uvcdata.callback = callback;
	req->context = &uvcdata;
	req->complete = uvc_data_complete;
}

static int uvc_vc_default_ctl(struct unit_control_info *info, u8 req, u8 *data)
{
	if (!(req & info->flags))
		return -1;

	switch (req) {
	case SET_CUR:
		uvc_data_setup(&info->cur, info->len, info->callback, 0);
		break;

	case GET_CUR:
		if (info->id == UVC_UNIT_EXT_ID && info->cs == XU_IMU_DATA) {
			spin_lock(&gUvcCtrldev->imulock);
			memcpy(data, &g_imudata, sizeof(struct imu_data));
			spin_unlock(&gUvcCtrldev->imulock);
		} else {
			memcpy(data, &info->cur, info->len);
		}
		break;

	case GET_MIN:
		memcpy(data, &info->min, info->len);
		break;

	case GET_MAX:
		memcpy(data, &info->max, info->len);
		break;

	case GET_RES:
		*(u16 *)data = info->res;
		break;

	case GET_DEF:
		memcpy(data, &info->def, info->len);
		break;

	case GET_INFO:
		*(u8 *)data = info->info;
		break;

	case GET_LEN:
		*(u16 *)data = info->len;
		break;

	default:
		printk("no such request\n");
		break;
	}

	return 0;
}

static int vc_mjpeg_probe_ctl(struct unit_control *uctl, const struct usb_ctrlrequest *ctrl, u8 *data)
{
	u16	w_length = le16_to_cpu(ctrl->wLength);
	int value = -1;

	switch (ctrl->bRequest) {
	case SET_CUR:
		value = min(w_length, (u16)sizeof(uvc_mjpeg_vpcc_cur));
		uvc_data_setup(&uvc_mjpeg_vpcc_cur, value, uvc_mjpeg_cmd_put, 0);
		break;

	case GET_CUR:
		value = min(w_length, (u16)sizeof(uvc_mjpeg_vpcc_cur));
		memcpy(data, &uvc_mjpeg_vpcc_cur, value);
		break;

	case GET_DEF:
		value = min(w_length, (u16)sizeof(uvc_mjpeg_vpcc_def));
		memcpy(data, &uvc_mjpeg_vpcc_def, value);
		break;

	case GET_MIN:
		value = min(w_length, (u16)sizeof(uvc_mjpeg_vpcc_min));
		memcpy(data, &uvc_mjpeg_vpcc_min, value);
		break;

	case GET_MAX:
		value = min(w_length, (u16)sizeof(uvc_mjpeg_vpcc_max));
		memcpy(data, &uvc_mjpeg_vpcc_max, value);
		break;

	case GET_INFO:
		*(u16 *)data = UVC_SUPPORT_GET_VALUE | UVC_SUPPORT_SET_VALUE;
		value = 1;
		break;

	default:
		printk("no such probe_ctl comand\n");
		break;
	}

	return value;
}

static int vc_mjpeg_commit_ctl(struct unit_control *uctl, const struct usb_ctrlrequest *ctrl, u8 *data)
{
	u16	w_length = le16_to_cpu(ctrl->wLength);
	int value = -1;
	switch (ctrl->bRequest) {
	case SET_CUR:
		value = min(w_length, (u16)sizeof(uvc_mjpeg_vpcc_cmt));
		uvc_data_setup(&uvc_mjpeg_vpcc_cmt, value, NULL, 0);
		break;

	case GET_CUR:
		value = min(w_length, (u16)sizeof(uvc_mjpeg_vpcc_cmt));
		memcpy(data, &uvc_mjpeg_vpcc_cmt, value);
		break;
	default:
		printk("no such commit_ctl comand\n");
		break;
	}

	return value;
}

static int uvc_vc_ctrl(struct unit_control *uctl, const struct usb_ctrlrequest *ctrl, u8 *data)
{
	struct unit_control_info *info = NULL;
	u16 w_index = le16_to_cpu(ctrl->wIndex);
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16	w_length = le16_to_cpu(ctrl->wLength);
	int value = -EOPNOTSUPP;

	value = uvc_unit_find_info(uctl, &info, UNIT_ID_GET(w_index), UNIT_CS_GET(w_value));
	if (value < 0)
		goto done;

	if (info->unit_uniq_ctl)
		value = info->unit_uniq_ctl(info, ctrl->bRequest, data);
	else
		goto done;

	value = w_length;

done:

	return value;
}

int uvc_class_cmd(const struct usb_ctrlrequest *ctrl)
{
	int value = -1;
	struct usb_request *req = media_get_dev()->req_ctrl;
	struct uvc_ctrl_device *ctrldev = gUvcCtrldev;
	u8 bifIdx = (u8)(ctrl->wIndex);
	u8 bcsIdx = UNIT_ID_GET(ctrl->wValue);

	int mjpeg_stream_status = 0;

	switch (bifIdx) {
	case UVC_INTFACE_STATUS:
		value = uvc_vc_ctrl(&ctrldev->unit_ctl, ctrl, req->buf);
		break;

	case MJPEG_INTFACE_STREAM:
		switch (bcsIdx) {
		case VS_CONTROL_UNDEFINED:
			break;

		case VS_STREAM_ERROR_CODE_CONTROL:
			*(u8 *) req->buf = 0x06;
			value = 1;
			break;

		case VS_PROBE_CONTROL:
			value = vc_mjpeg_probe_ctl(&ctrldev->unit_ctl, ctrl, req->buf);
			mjpeg_stream_status = get_mjpeg_stream_status();
			if (mjpeg_stream_status == 1) {
				uvc_media_cmd_put(DEVICE_PW_SV, 0);
				mdelay(10);
				mjpeg_config_reset();
			}
			break;

		case VS_COMMIT_CONTROL:
			value = vc_mjpeg_commit_ctl(&ctrldev->unit_ctl, ctrl, req->buf);
			mjpeg_stream_status = get_mjpeg_stream_status();
			if (mjpeg_stream_status == 0) {
				uvc_media_cmd_put(DEVICE_PW_ON, 0);
				mdelay(10);
				mjpeg_stream_start(gMedia->gadget, GFP_ATOMIC);
			}
			break;

		default:
			printk("Req for VStreaming, value:%x\n", bcsIdx);
			break;
		}
		break;

	default:
		printk("***no such interface\n");
		break;
	}
	return value;
}

static int uvc_unit_ctrl_add_info(struct unit_control *uctl, struct unit_control_info *info)
{
	list_add_tail(&info->list, &uctl->controls);
	return 0;
}

static int uvc_unit_find_info(struct unit_control *uctl, struct unit_control_info **info, u8 id, u8 cs)
{
	struct unit_control_info *linfo = NULL;

	list_for_each_entry(linfo, &uctl->controls, list) {
		if (linfo->id == id && linfo->cs == cs) {
			*info = linfo;
			return 0;
		}
	}

	*info = NULL;
	return -1;
}

static int uvc_ctrl_unit_init(struct unit_control *uctl)
{
	int retval = 0;
	int size, i;
	struct unit_control_info *info;

	memset(uctl, 0, sizeof(*uctl));

	INIT_LIST_HEAD(&uctl->controls);

	size = ARRAY_SIZE(uvc_ctrls);
	info = uvc_ctrls;

	for (i = 0; i < size; i++) {
		retval = uvc_unit_ctrl_add_info(uctl, info++);
	}

	return retval;

}


static int uvc_ctrl_unit_cancel(struct unit_control *uctl)
{
	int retval = 0;
	INIT_LIST_HEAD(&uctl->controls);
	return retval;
}

static int uvc_ctrl_open(struct inode *inode, struct file *filp)
{
	int retval = 0;
	filp->private_data = gUvcCtrldev;
	return retval;
}

static int uvc_ctrl_release(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;
	return 0;
}

static int write_imudata(struct uvc_ctrl_device *ctrldev, struct imu_data* data)
{
	spin_lock(&ctrldev->imulock);
	memcpy(&g_imudata, data, sizeof(struct imu_data));
	spin_unlock(&ctrldev->imulock);
	return 0;
}

static long uvc_ctrl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct uvc_ctrl_device *ctrldev = filp->private_data;
	struct imu_data* imudata = NULL;
	int retval = 0;

	switch (cmd) {
	case CTRL_SET_CMD:
		imudata = (struct imu_data*)arg;
		write_imudata(ctrldev, imudata);
		break;

	case CTRL_GET_CMD:
		retval = uvc_cmd_get(&ctrldev->vd_queue, (void *)arg);
		if (retval != 0) {
			printk("CTRL_GET_CMD Faild \n");
		}
		break;

	default:
		printk("uvc_ctrl_ioctl err\n");
		retval = -1;
		break;
	}

	return retval;
}

static struct file_operations uvc_ctrl_fops = {
	.owner 	 		= THIS_MODULE,
	.open  	 		= uvc_ctrl_open,
	.release 		= uvc_ctrl_release,
	.unlocked_ioctl = uvc_ctrl_ioctl,
};

int uvc_ctrl_init(struct usb_gadget *gadget)
{
	int retval = 0;

	struct uvc_ctrl_device *ctrldev = NULL;

	ctrldev = kzalloc(sizeof(*ctrldev), GFP_KERNEL);
	if (!ctrldev) {
		printk("uvc kzalloc uvc control dev memory failed \n");
		return -ENOMEM;
	}

	gUvcCtrldev = ctrldev;
	spin_lock_init(&ctrldev->imulock);
	uvc_mjpeg_vpcc_cmt = uvc_mjpeg_vpcc_cur;

	uvc_ctrl_unit_init(&ctrldev->unit_ctl);
	retval = drvbuf_queue_init(&ctrldev->vd_queue, 6, sizeof(struct uvccmd_buffer), DRVBUF_Q_BLOCK);
	if (retval < 0) {
		printk("drvbuf_queue_init video cmd queue init failed\n");
		goto done;
	}

	retval = ezybuf_queue_init(&ctrldev->ctrl_queue, &uvc_ctrl_fops, sizeof(struct ezybuf_buffer), 0, CTRL_DRIVER_NAME, ctrldev);
	if (retval < 0) {
		printk("ezybuf_queue_ini fail!\n");
		goto drvbuf_done;
	}

	return 0;

drvbuf_done:
	drvbuf_queue_cancel(&ctrldev->vd_queue);
done:
	if (ctrldev)
		kfree(ctrldev);
	return -1;
}

void uvc_ctrl_uninit(void)
{
	struct uvc_ctrl_device *ctrldev = gUvcCtrldev;

	ezybuf_queue_cancel(&ctrldev->ctrl_queue);
	drvbuf_queue_cancel(&ctrldev->vd_queue);
	uvc_ctrl_unit_cancel(&ctrldev->unit_ctl);

	if (gUvcCtrldev)
		kfree(gUvcCtrldev);

	gUvcCtrldev = NULL;

	printk("uvc ctrl funciotn uninit finished \n");
}
