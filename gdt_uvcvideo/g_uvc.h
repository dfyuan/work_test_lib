#ifndef _G_UVC_VIDEO_H_
#define _G_UVC_VIDEO_H_

#include <linux/kernel.h>
#include <linux/videodev2.h>
#include "sysconfig.h"

/* Payload Header Definition */
#define UVC_FID		0x01
#define UVC_EOF		0x02
#define UVC_PTS		0x04
#define UVC_SCR		0x08
#define UVC_RES		0x10
#define UVC_STI		0x20
#define UVC_ERR		0x40
#define UVC_EOH		0x80

struct uvc_stream_header {
	__u8  bHeaderLength;
	__u8  bmHeaderInfo;
	__u32 dwPresentationTime;
	__u64 scrSourceClock: 48;
} __attribute__((packed));




#define UNIT_ID_GET(unit)		(unit >> 8)
#define UNIT_CS_GET(unit)		(unit >> 8)

#define UVC_UNIT_PROCOM_ID			0X00
#define UVC_TERM_CAM_ID				0X01
#define UVC_UNIT_PUNIT_ID			0X02
#define UVC_UNIT_EXT_ID				0X03
#define UVC_TERM_OTT_ID_MJPEG		0X05


#define USB_CLASS_MISC			0xef

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

#define UVC_CONTROL_GET_RANGE	(UVC_CONTROL_GET_CUR | UVC_CONTROL_GET_MIN | \
				 UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_RES | \
				 UVC_CONTROL_GET_DEF)

/*  Defined Bits Containing Capabilities of the Control */
#define UVC_SUPPORT_GET_VALUE	(1 << 0)
#define UVC_SUPPORT_SET_VALUE	(1 << 1)
#define UVC_SUPPORT_AUTO_MODE	(1 << 2)
#define UVC_SUPPORT_AUTO_CTRL 	(1 << 3)
#define UVC_SUPPORT_ASYNC_CTRL  (1 << 4)

#define SC_UNDEFINED			0x00
#define SC_VIDEOCONTROL			0x01
#define SC_VIDEOSTREAMING		0x02
#define SC_VIDEO_INTERFACE_COLLECTION	0x03

#define PC_PROTOCOL_UNDEFINED	0x00

#define CS_UNDEFINED			0x20
#define CS_DEVICE				0x21
#define CS_CONFIGURATION		0x22
#define CS_STRING				0x23
#define CS_INTERFACE			0x24
#define CS_ENDPOINT				0x25

/* VideoControl class specific interface descriptor */
#define VC_DESCRIPTOR_UNDEFINED	0x00
#define VC_HEADER				0x01
#define VC_INPUT_TERMINAL		0x02
#define VC_OUTPUT_TERMINAL		0x03
#define VC_SELECTOR_UNIT		0x04
#define VC_PROCESSING_UNIT		0x05
#define VC_EXTENSION_UNIT		0x06

/* VideoStreaming class specific interface descriptor */
#define VS_UNDEFINED			0x00
#define VS_INPUT_HEADER			0x01
#define VS_OUTPUT_HEADER		0x02
#define VS_STILL_IMAGE_FRAME	0x03
#define VS_FORMAT_UNCOMPRESSED	0x04
#define VS_FRAME_UNCOMPRESSED	0x05
#define VS_FORMAT_MJPEG			0x06
#define VS_FRAME_MJPEG			0x07
#define VS_FORMAT_MPEG2TS		0x0a
#define VS_FORMAT_DV			0x0c
#define VS_COLORFORMAT			0x0d
#define VS_FORMAT_FRAME_BASED	0x10
#define VS_FRAME_FRAME_BASED	0x11
#define VS_FORMAT_STREAM_BASED	0x12

/* Endpoint type */
#define EP_UNDEFINED			0x00
#define EP_GENERAL				0x01
#define EP_ENDPOINT				0x02
#define EP_INTERRUPT			0x03

/* Request codes */
#define RC_UNDEFINED		0x00
#define SET_CUR				0x01
#define SET_MIN				0x02
#define SET_MAX				0x03
#define SET_RES				0x04
#define SET_LEN				0x05
#define SET_INFO			0x06
#define SET_DEF				0x07
#define GET_CUR				0x81
#define GET_MIN				0x82
#define GET_MAX				0x83
#define GET_RES				0x84
#define GET_LEN				0x85
#define GET_INFO			0x86
#define GET_DEF				0x87

/* VideoControl interface controls */
#define VC_CONTROL_UNDEFINED			0x00
#define VC_VIDEO_POWER_MODE_CONTROL		0x01
#define VC_REQUEST_ERROR_CODE_CONTROL	0x02

/* Terminal controls */
#define TE_CONTROL_UNDEFINED		0x00

/* Selector Unit controls */
#define SU_CONTROL_UNDEFINED		0x00
#define SU_INPUT_SELECT_CONTROL		0x01

/* Camera Terminal controls */
#define CT_CONTROL_UNDEFINED				0x00
#define CT_SCANNING_MODE_CONTROL			0x01
#define CT_AE_MODE_CONTROL					0x02
#define CT_AE_PRIORITY_CONTROL				0x03
#define CT_EXPOSURE_TIME_ABSOLUTE_CONTROL	0x04
#define CT_EXPOSURE_TIME_RELATIVE_CONTROL	0x05
#define CT_FOCUS_ABSOLUTE_CONTROL			0x06
#define CT_FOCUS_RELATIVE_CONTROL			0x07
#define CT_FOCUS_AUTO_CONTROL				0x08
#define CT_IRIS_ABSOLUTE_CONTROL			0x09
#define CT_IRIS_RELATIVE_CONTROL			0x0a
#define CT_ZOOM_ABSOLUTE_CONTROL			0x0b
#define CT_ZOOM_RELATIVE_CONTROL			0x0c
#define CT_PANTILT_ABSOLUTE_CONTROL			0x0d
#define CT_PANTILT_RELATIVE_CONTROL			0x0e
#define CT_ROLL_ABSOLUTE_CONTROL			0x0f
#define CT_ROLL_RELATIVE_CONTROL			0x10
#define CT_PRIVACY_CONTROL					0x11

/* Processing Unit controls */
#define PU_CONTROL_UNDEFINED						0x00
#define PU_BACKLIGHT_COMPENSATION_CONTROL			0x01
#define PU_BRIGHTNESS_CONTROL						0x02
#define PU_CONTRAST_CONTROL							0x03
#define PU_GAIN_CONTROL								0x04
#define PU_POWER_LINE_FREQUENCY_CONTROL				0x05
#define PU_HUE_CONTROL								0x06
#define PU_SATURATION_CONTROL						0x07
#define PU_SHARPNESS_CONTROL						0x08
#define PU_GAMMA_CONTROL							0x09
#define PU_WHITE_BALANCE_TEMPERATURE_CONTROL		0x0a
#define PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL	0x0b
#define PU_WHITE_BALANCE_COMPONENT_CONTROL			0x0c
#define PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL		0x0d
#define PU_DIGITAL_MULTIPLIER_CONTROL				0x0e
#define PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL			0x0f
#define PU_HUE_AUTO_CONTROL							0x10
#define PU_ANALOG_VIDEO_STANDARD_CONTROL			0x11
#define PU_ANALOG_LOCK_STATUS_CONTROL				0x12

#define LXU_MOTOR_PANTILT_RELATIVE_CONTROL			0x01
#define LXU_MOTOR_PANTILT_RESET_CONTROL				0x02
#define LXU_MOTOR_FOCUS_MOTOR_CONTROL				0x03

/* Extension Unit Control Selectors */
#define XU_CONTROL_UNDEFINED       					0x00
#define XU_VIDEO_MODE       						0x01
#define XU_DISK_MODE       							0x02
#define XU_BURNING_MODE       						0x03
#define XU_IMU_DATA      							0x0c


/* VideoStreaming interface controls */
#define VS_CONTROL_UNDEFINED			0x00
#define VS_PROBE_CONTROL				0x01
#define VS_COMMIT_CONTROL				0x02
#define VS_STILL_PROBE_CONTROL			0x03
#define VS_STILL_COMMIT_CONTROL			0x04
#define VS_STILL_IMAGE_TRIGGER_CONTROL	0x05
#define VS_STREAM_ERROR_CODE_CONTROL	0x06
#define VS_GENERATE_KEY_FRAME_CONTROL	0x07
#define VS_UPDATE_FRAME_SEGMENT_CONTROL	0x08
#define VS_SYNC_DELAY_CONTROL			0x09

#define TT_VENDOR_SPECIFIC				0x0100
#define TT_STREAMING					0x0101

/* Input Terminal types */
#define ITT_VENDOR_SPECIFIC			0x0200
#define ITT_CAMERA					0x0201
#define ITT_MEDIA_TRANSPORT_INPUT	0x0202

/* Output Terminal types */
#define OTT_VENDOR_SPECIFIC			0x0300
#define OTT_DISPLAY					0x0301
#define OTT_MEDIA_TRANSPORT_OUTPUT	0x0302

/* External Terminal types */
#define EXTERNAL_VENDOR_SPECIFIC	0x0400
#define COMPOSITE_CONNECTOR			0x0401
#define SVIDEO_CONNECTOR			0x0402
#define COMPONENT_CONNECTOR			0x0403

#define UVC_TERM_INPUT				0x0000
#define UVC_TERM_OUTPUT				0x8000

#define UVC_ENTITY_TYPE(entity)		((entity)->type & 0x7fff)
#define UVC_ENTITY_IS_UNIT(entity)	(((entity)->type & 0xff00) == 0)
#define UVC_ENTITY_IS_TERM(entity)	(((entity)->type & 0xff00) != 0)
#define UVC_ENTITY_IS_ITERM(entity) \
	(((entity)->type & 0x8000) == UVC_TERM_INPUT)
#define UVC_ENTITY_IS_OTERM(entity) \
	(((entity)->type & 0x8000) == UVC_TERM_OUTPUT)

#define UVC_STATUS_TYPE_CONTROL		1
#define UVC_STATUS_TYPE_STREAMING	2
#define UVC_HLE		12	/* header length  12 */


/* ------------------------------------------------------------------------
 * GUIDs
 */

#define UVC_GUID_FORMAT_MJPEG \
	{ 'M',  'J',  'P',  'G', 0x00, 0x00, 0x10, 0x00, \
	 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
#define UVC_GUID_FORMAT_YUY2 \
	{ 'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00, \
	 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
#define UVC_GUID_FORMAT_NV12 \
	{ 'N',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00, \
	 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
#define UVC_GUID_FORMAT_YV12 \
	{ 'Y',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00, \
	 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
#define UVC_GUID_FORMAT_I420 \
	{ 'I',  '4',  '2',  '0', 0x00, 0x00, 0x10, 0x00, \
	 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
#define UVC_GUID_FORMAT_UYVY \
	{ 'U',  'Y',  'V',  'Y', 0x00, 0x00, 0x10, 0x00, \
	 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
#define UVC_GUID_FORMAT_Y800 \
	{ 'Y',  '8',  '0',  '0', 0x00, 0x00, 0x10, 0x00, \
	 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
#define UVC_GUID_FORMAT_BY8 \
	{ 'B',  'Y',  '8',  ' ', 0x00, 0x00, 0x10, 0x00, \
	 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}

#define UVC_GUID_FORMAT_MPEG \
	{ 'M',  'P',  'E',  'G', 0x00, 0x00, 0x10, 0x00, \
	 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}
#define UVC_GUID_EMPTY		\
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define UVC_GUID_EXTEN		\
	{ 0x56 ,0x4C, 0x97, 0xA7, 0x7E, 0xA7, 0x90, 0x4B,\
	 0x8C, 0xBF, 0x1C, 0x71, 0xEC, 0x30, 0x30, 0x00}
#define UVC_GUID_UVC_SUNNY_EXT \
	{'S', 'U', 'N', 'N', 'Y', 0x00, 0x00, 0x00, \
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}



#define UVC_GUID_FORMAT "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-" \
			"%02x%02x%02x%02x%02x%02x"
#define UVC_GUID_ARGS(guid) \
	(guid)[3],  (guid)[2],  (guid)[1],  (guid)[0], \
	(guid)[5],  (guid)[4], \
	(guid)[7],  (guid)[6], \
	(guid)[8],  (guid)[9], \
	(guid)[10], (guid)[11], (guid)[12], \
	(guid)[13], (guid)[14], (guid)[15]


/* ------------------------------------------------------------------------
 * Driver specific constants.
 */

#define DRIVER_VERSION_NUMBER	KERNEL_VERSION(0, 1, 0)

/* Number of isochronous URBs. */
#define UVC_URBS		5
/* Maximum number of packets per isochronous URB. */
#define UVC_MAX_ISO_PACKETS	40
/* Maximum frame size in bytes, for sanity checking. */
#define UVC_MAX_FRAME_SIZE	(16*1024*1024)
/* Maximum number of video buffers. */
#define UVC_MAX_VIDEO_BUFFERS	32
/* Maximum status buffer size in bytes of interrupt URB. */
#define UVC_MAX_STATUS_SIZE	16

#define UVC_CTRL_CONTROL_TIMEOUT	300
#define UVC_CTRL_STREAMING_TIMEOUT	1000

/* Devices quirks */
#define UVC_QUIRK_STATUS_INTERVAL	0x00000001
#define UVC_QUIRK_PROBE_MINMAX		0x00000002
#define UVC_QUIRK_PROBE_EXTRAFIELDS	0x00000004
#define UVC_QUIRK_BUILTIN_ISIGHT	0x00000008
#define UVC_QUIRK_STREAM_NO_FID		0x00000010
#define UVC_QUIRK_IGNORE_SELECTOR_UNIT	0x00000020
#define UVC_QUIRK_PRUNE_CONTROLS	0x00000040

/* Format flags */
#define UVC_FMT_FLAG_COMPRESSED		0x00000001
#define UVC_FMT_FLAG_STREAM		0x00000002

#define VC_INTERFACE_COLLECTION_NB	1

/* Class-Specific Video Control Interface Header Descriptor */
struct uvc_vc_interface_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__le16 bcdUVC;
	__le16 wTotalLength;
	__le32 dwClockFrequency;
	__u8 bInCollection;
	__u8 baInterfaceNr[VC_INTERFACE_COLLECTION_NB];	/*  just one */
} __attribute__((packed));

#define VC_ITTC_CONTROL_SIZE	3

/* Video Control Input Terminal Descriptor */
struct uvc_vc_ittc_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bTerminalID;
	__le16 wTerminalType;
	__u8 bAssocTerminal;
	__u8 iTerminal;
	__le16 wObjectiveFocalLengthMin;
	__le16 wObjectiveFocalLengthMax;
	__le16 wOcularFocalLength;
	__u8 bControlSize;
	__u8 bmControls[VC_ITTC_CONTROL_SIZE];
} __attribute__((packed));


#define VC_PUNIT_CONTROL_SIZE	2

/* Video Control Processing Unit Descriptor */
struct uvc_vc_punit_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bUnitID;
	__u8 bSourceID;
	__le16 wMaxMultiplier;
	__u8 bControlSize;
	__u8 bmControls[VC_PUNIT_CONTROL_SIZE];
	__u8 iProcessing;
	__u8 bmVideoStandards;
} __attribute__((packed));


/* Video Control Output Terminal Descriptor */
struct uvc_vc_ott_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bTerminalID;
	__le16 wTerminalType;
	__u8 bAssocTerminal;
	__u8 bSourceID;
	__u8 iTerminal;
} __attribute__((packed));


/* Class-specific VC Interrupt Endpoint Descriptor */
struct uvc_vc_intendpoint_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__le16 wMaxTransferSize;
} __attribute__((packed));

#define VC_EXT_UNIT_NR_CONTROLS		0x0d
#define VC_EXT_UNIT_EXTENSION		0x0
#define VC_EXT_UNIT_NR_IN_PINS		1
#define VC_EXT_UNIT_CONTROL_SIZE	0x02

struct uvc_vc_extension_unit_desciptor {
	__u8  bLength;                  /* 24+p+n bytes */
	__u8  bDescriptorType;          /* CS_INTERFACE */
	__u8  bDescriptorSubType;       /* VC_EXTENSION_UNIT */
	__u8  bUnitID;
	__u8  guidExtensionCode[16];
	__u8  bNumControls;
	__u8  bNrInPins;                /* p */
	__u8  baSourceID[VC_EXT_UNIT_NR_IN_PINS];
	__u8  bControlSize;             /* n */
	__u8  bmControls[VC_EXT_UNIT_CONTROL_SIZE];
	__u8  iExtension;
} __attribute__((packed));


/* Class-Specific Video Input Interface Header Descriptor */
struct uvc_vs_iptinterface_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bNumFormats;
	__le16 wTotalLength;
	__u8 bEndpointAddress;
	__u8 bmInfo;
	__u8 bTerminalLink;
	__u8 bStillCaptureMethod;
	__u8 bTriggerSupport;
	__u8 bTriggerUsage;
	__u8 bControlSize;
	__u8 bmaControls[2];
} __attribute__((packed));


/* Video Stream Based Format Descriptor */
struct uvc_vs_stream_format_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bFormatIndex;
	__u8 guidFormat[16];
	__le32 dwPacketLength;
} __attribute__((packed));


/* Video Frame Based Format Type Descriptor */
struct uvc_vs_frame_format_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bFormatIndex;
	__u8 bNumFrameDescriptors;
	__u8 guidFormat[16];
	__u8 bBitsPerPixel;
	__u8 bDefaultFrameIndex;
	__u8 bAspectRatioX;
	__u8 bAspectRatioY;
	__u8 bmInterlaceFlags;
	__u8 bCopyProtect;
	__u8 bVariableSize;
} __attribute__((packed));

#define VS_FRAME_FRAME_INTERVAL_NB		3 //5

/* Video Frame Based  Frame Type Descriptor */
struct uvc_vs_frame_frame_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bFrameIndex;
	__u8 bmCapabilities;
	__le16 wWidth;
	__le16 wHeight;
	__le32 dwMinBitRate;
	__le32 dwMaxBitRate;
	__le32 dwDefaultFrameInterval;
	__u8 bFrameIntervalType;
	__le32 dwBytesPerLine;
	__le32 dwFrameInterval[VS_FRAME_FRAME_INTERVAL_NB];
} __attribute__((packed));


/* Video Streaming Uncompressed Format Type Descriptor */
struct uvc_vs_uncompres_format_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bFormatIndex;
	__u8 bNumFrameDescriptors;
	__u8 guidFormat[16];
	__u8 bBitsPerPixel;
	__u8 bDefaultFrameIndex;
	__u8 bAspectRatioX;
	__u8 bAspectRatioY;
	__u8 bmInterlaceFlags;
	__u8 bCopyProtect;
} __attribute__((packed));

/* Video Streaming Uncompressed Frame Type Descriptor */
struct uvc_vs_uncompres_frame_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bFrameIndex;
	__u8 bmCapabilities;
	__le16 wWidth;
	__le16 wHeight;
	__le32 dwMinBitRate;
	__le32 dwMaxBitRate;
	__le32 dwMaxVideoFrameBufferSize;
	__le32 dwDefaultFrameInterval;
	__u8 bFrameIntervalType;
	//__le32 dwMinFrameInterval;
	//__le32 dwMaxFrameInterval;
	//__le32 dwFrameIntervalStep;
	__le32 dwFrameInterval[3];
} __attribute__((packed));


/* Video MPEG2TS Format Type Descriptor */
struct uvc_vs_mpeg2ts_format_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bFormatIndex;
	__u8 bDataOffset;
	__u8 bPacketLength;
	__u8 bStrideLength;
	__u8 guidStrideFormat[16];	// uvc1.1 spec
} __attribute__((packed));


/* Video MJPEG Format Type Descriptor */
struct uvc_vs_mjpeg_format_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bFormatIndex;
	__u8 bNumFrameDescriptors;
	__u8 bmFlags;
	__u8 bDefaultFrameIndex;
	__u8 bAspectRatioX;
	__u8 bAspectRatioY;
	__u8 bmInterlaceFlags;
	__u8 bCopyProtect;
} __attribute__((packed));


#define VS_MJPEG_FRAME_INTERVAL_NB		3

/* Video MJPEG  Frame Type Descriptor */
struct uvc_vs_mjpeg_frame_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bFrameIndex;
	__u8 bmCapabilities;
	__le16 wWidth;
	__le16 wHeight;
	__le32 dwMinBitRate;
	__le32 dwMaxBitRate;
	__le32 dwMaxVideoFrameBufferSize;
	__le32 dwDefaultFrameInterval;
	__u8 bFrameIntervalType;
	__le32 dwFrameInterval[VS_MJPEG_FRAME_INTERVAL_NB];
} __attribute__((packed));


struct uvc_video_probecomit_controls {
	__le16 bmHint;
	__u8 bFormatIndex;
	__u8 bFrameIndex;
	__le32 dwFrameInterval;
	__le16 wKeyFrameRate;
	__le16 wPFrameRate;
	__le16 wCompQuality;
	__le16 wCompWindowSize;
	__le16 wDelay;	// ms unit
	__le32 dwMaxVideoFrameSize;
	__le32 dwMaxPayloadTransferSize;
	__le32 dwClockFrequency;
	__u8 bmFramingInfo;
	__u8 bPreferedVersion;
	__u8 bMinVersion;
	__u8 bMaxVersion;
} __attribute__((packed));


/* UVC Still Image Probe/Commit*/
struct uvc_stillimg_probecomit_control {
	__u8  bFormatIndex;
	__u8  bFrameIndex;
	__u8  bCompressionIndex;
	__u32 dwMaxVideoFrameSize;
	__u32 dwMaxPayloadTransferSize;
} __attribute__((packed));


#define  STILL_IMAGE_FRAME_SIZE_NB		1

struct uvc_still_image_frame_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtyp;
	__u8 bEndpointAddress;
	__u8 bNumImageSizePatterns;
	__le16 wWidth[STILL_IMAGE_FRAME_SIZE_NB];
	__le16 wHeight[STILL_IMAGE_FRAME_SIZE_NB];
	__u8 bNumCompressionPattern;
} __attribute__((packed));

#endif

