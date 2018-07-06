

/*
 * Hidraw Userspace Example
 *
 * Copyright (c) 2010 Alan Ott <alan@signal11.us>
 * Copyright (c) 2010 Signal 11 Software
 *
 * The code may be used by anyone for any purpose,
 * and can serve as a starting point for developing
 * applications using hidraw.
 */

/* Linux */
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

/*
 * Ugly hack to work around failing compilation on systems that don't
 * yet populate new version of hidraw.h to userspace.
 */
//#ifndef HIDIOCSFEATURE
//#warning Please have your distro update the userspace kernel headers
#define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
#define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
//#endif

/* Unix */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>


//report id
#define REPROT_ID 	0x04

//cmd1
#define NR_CMD 			0x01
#define WB_CMD      	0x02
#define AE_CMD 	 		0x03
#define SATURATION_CMD 	0x04
#define SHARPNESS_CMD 	0x05
#define ZOOM_IN 		0x06
#define ZOOM_OUT		0x07
#define ZOOM_STOP		0x08

//ae cmd2
#define AE_AUTO			0x01
#define SHUTTER_PRI		0x02
#define IRIS_PRI 		0x03
#define GAIN_PRI 		0x04

//wb cmd2
#define AUTO_WB 		0x01
#define INDOOR_WB 		0x02
#define OUTDOOR_WB 		0x03
#define MANUL_WB 		0x04





struct HID_DATA
{
 unsigned char report_id;
 unsigned char cmd1;
 unsigned char cmd2;
 unsigned char reserved;
 unsigned int data0;
 unsigned int data1;
 unsigned int data2;
};

const char *bus_str(int bus);

static int show_menu(void)
{
	printf("\n");
	printf("|-------------------------------------|\n");
	printf("| a  -	ae  controle                  |\n");
	printf("| i  -	iris  controle                |\n");
	printf("| t  -	shutter controle              |\n");
	printf("| g  -	gain controle                 |\n");
	printf("| n  -	nosie controle                |\n");
	printf("| w  -	wb controle               	  |\n");
	printf("| b  -	sat controle           		  |\n");
	printf("| s  -	sharp controle           	  |\n");
	printf("| x  -	zoom in controle              |\n");
	printf("| y  -	zoom out controle             |\n");
	printf("| z  -	zoom stop controle            |\n");
	printf("| q  - Quit                           |\n");
	printf("|-------------------------------------|\n");
	printf("\n");
	return 0;
}
static void ae_controle(int fd )
{
	struct HID_DATA hiddata;
	unsigned int data;
	int res=0;
	//printf("please input the value:(0-32 default 0 for auto)\n");
	//scanf("%d" ,&data);
	memset(&hiddata,0,sizeof(struct HID_DATA));
	hiddata.report_id=REPROT_ID;
	hiddata.cmd1=AE_CMD;
	hiddata.cmd2=AE_AUTO;
	//hiddata.data0=data;
	printf("sizeof(struct HID_DATA)=%ld \n",sizeof(struct HID_DATA));
	res = write(fd, &hiddata, sizeof(struct HID_DATA));
	if (res < 0)
	{
		printf("Error: %d\n", errno);
		perror("write");
	} 
	else
	{
		printf("write() wrote %d bytes\n", res);
	}
}
static void iris_controle(int fd )
{
	struct HID_DATA hiddata;
	unsigned int data;
	int res=0;
	printf("please input the value:(0-15 default (0:Open 15:Close))\n");
	scanf("%d" ,&data);
	memset(&hiddata,0,sizeof(struct HID_DATA));
	hiddata.report_id=REPROT_ID;
	hiddata.cmd1=AE_CMD;
	hiddata.cmd2=IRIS_PRI;
	hiddata.data0=data;
	printf("sizeof(struct HID_DATA)=%ld \n",sizeof(struct HID_DATA));
	res = write(fd, &hiddata, sizeof(struct HID_DATA));
	if (res < 0)
	{
		printf("Error: %d\n", errno);
		perror("write");
	} 
	else
	{
		printf("write() wrote %d bytes\n", res);
	}
}
static void shutter_controle(int fd )
{
	struct HID_DATA hiddata;
	unsigned int data;
	int res=0;
	printf("please input the value:(10-8000 default 0 for auto)\n");
	scanf("%d" ,&data);
	memset(&hiddata,0,sizeof(struct HID_DATA));
	hiddata.report_id=REPROT_ID;
	hiddata.cmd1=AE_CMD;
	hiddata.cmd2=SHUTTER_PRI;
	hiddata.data0=data;
	printf("sizeof(struct HID_DATA)=%ld \n",sizeof(struct HID_DATA));
	res = write(fd, &hiddata, sizeof(struct HID_DATA));
	if (res < 0)
	{
		printf("Error: %d\n", errno);
		perror("write");
	} 
	else
	{
		printf("write() wrote %d bytes\n", res);
	}
}
static void gain_controle(int fd )
{
	struct HID_DATA hiddata;
	unsigned int data;
	int res=0;
	printf("please input the value:(0-32 default 0 for auto)\n");
	scanf("%d" ,&data);
	memset(&hiddata,0,sizeof(struct HID_DATA));
	hiddata.report_id=REPROT_ID;
	hiddata.cmd1=AE_CMD;
	hiddata.cmd2=GAIN_PRI;
	hiddata.data0=data;
	printf("sizeof(struct HID_DATA)=%ld \n",sizeof(struct HID_DATA));
	res = write(fd, &hiddata, sizeof(struct HID_DATA));
	if (res < 0)
	{
		printf("Error: %d\n", errno);
		perror("write");
	} 
	else
	{
		printf("write() wrote %d bytes\n", res);
	}
}

static void nr_controle(int fd)
{
	struct HID_DATA hiddata;
	unsigned int data;
	int res=0;

	printf("please input the value:\n");
	scanf("%d" ,&data);

	
	memset(&hiddata,0,sizeof(struct HID_DATA));
	hiddata.report_id=REPROT_ID;
	hiddata.cmd1=NR_CMD;
	hiddata.cmd2=0;
	hiddata.data0=data;
	
	res = write(fd, &hiddata, sizeof(struct HID_DATA));
	if (res < 0)
	{
		printf("Error: %d\n", errno);
		perror("write");
	} 
	else
	{
		printf("write() wrote %d bytes\n", res);
	}
}

static void wb_controle(int fd)
{
	struct HID_DATA hiddata;
	unsigned int data;
	int res=0;

	printf("please input the value:\n");
	scanf("%d",&data);

	memset(&hiddata,0,sizeof(struct HID_DATA));
	hiddata.report_id=REPROT_ID;
	hiddata.cmd1=WB_CMD;
	hiddata.cmd2=data;

	if(data==0x04)
	{
		printf("please input the gain: Rgain Bgain Ggain\n");
		scanf("%d %d %d",&(hiddata.data0),&(hiddata.data1),&(hiddata.data2));
	}
	
	res = write(fd, &hiddata, sizeof(struct HID_DATA));
	if (res < 0)
	{
		printf("Error: %d\n", errno);
		perror("write");
	} 
	else
	{
		printf("write() wrote %d bytes\n", res);
	}
}
static void sat_controle(int fd)
{
	struct HID_DATA hiddata;
	unsigned int data;
	int res=0;
	memset(&hiddata,0,sizeof(struct HID_DATA));

	printf("please input the value:(0~15)\n");
	scanf("%d",&(hiddata.data0));

	
	hiddata.report_id=REPROT_ID;
	hiddata.cmd1=SATURATION_CMD;
	
	res = write(fd, &hiddata, sizeof(struct HID_DATA));
	if (res < 0)
	{
		printf("Error: %d\n", errno);
		perror("write");
	} 
	else
	{
		printf("write() wrote %d bytes\n", res);
	}
}
static void sharp_controle(int fd)
{
	struct HID_DATA hiddata;
	unsigned int data;
	int res=0;
	memset(&hiddata,0,sizeof(struct HID_DATA));

	printf("please input the value:(0~11 default :6)\n");
	scanf("%d",&(hiddata.data0));

	
	hiddata.report_id=REPROT_ID;
	hiddata.cmd1=SHARPNESS_CMD;
	
	res = write(fd, &hiddata, sizeof(struct HID_DATA));
	if (res < 0)
	{
		printf("Error: %d\n", errno);
		perror("write");
	} 
	else
	{
		printf("write() wrote %d bytes\n", res);
	}
}

static void zoomin_controle(int fd)
{
	struct HID_DATA hiddata;
	unsigned int data;
	int res=0;
	memset(&hiddata,0,sizeof(struct HID_DATA));

	printf("please input the value:(0~7 default :7)\n");
	scanf("%d",&(hiddata.data0));

	
	hiddata.report_id=REPROT_ID;
	hiddata.cmd1=ZOOM_IN;
	
	res = write(fd, &hiddata, sizeof(struct HID_DATA));
	if (res < 0)
	{
		printf("Error: %d\n", errno);
		perror("write");
	} 
	else
	{
		printf("write() wrote %d bytes\n", res);
	}
}
static void zoomout_controle(int fd)
{
	struct HID_DATA hiddata;
	unsigned int data;
	int res=0;
	memset(&hiddata,0,sizeof(struct HID_DATA));

	printf("please input the value:(0~7 default :7)\n");
	scanf("%d",&(hiddata.data0));

	
	hiddata.report_id=REPROT_ID;
	hiddata.cmd1=ZOOM_OUT;
	
	res = write(fd, &hiddata, sizeof(struct HID_DATA));
	if (res < 0)
	{
		printf("Error: %d\n", errno);
		perror("write");
	} 
	else
	{
		printf("write() wrote %d bytes\n", res);
	}
}
static void zoomstop_controle(int fd)
{
	struct HID_DATA hiddata;
	unsigned int data;
	int res=0;
	memset(&hiddata,0,sizeof(struct HID_DATA));

	hiddata.report_id=REPROT_ID;
	hiddata.cmd1=ZOOM_STOP;
	
	res = write(fd, &hiddata, sizeof(struct HID_DATA));
	if (res < 0)
	{
		printf("Error: %d\n", errno);
		perror("write");
	} 
	else
	{
		printf("write() wrote %d bytes\n", res);
	}
}

int main(int argc, char **argv)
{
	int fd;
	int i, res, desc_size = 0;
	char buf[256];
	struct hidraw_report_descriptor rpt_desc;
	struct hidraw_devinfo info;
	//char cmd;
	char ch,quit_flag=0,error_opt=0;
	int data;

	/* Open the Device with non-blocking reads. In real life,
	   don't use a hard coded path; use libudev instead. */
	fd = open("/dev/hidraw0", O_RDWR|O_NONBLOCK);
	if (fd < 0)		
	{
		perror("Unable to open device");
		return 1;
	}

	memset(&rpt_desc, 0x0, sizeof(rpt_desc));
	memset(&info, 0x0, sizeof(info));
	memset(buf, 0x0, sizeof(buf));

	/* Get Report Descriptor Size */
	res = ioctl(fd, HIDIOCGRDESCSIZE, &desc_size);
	if (res < 0)
		perror("HIDIOCGRDESCSIZE");
	else
		printf("Report Descriptor Size: %d\n", desc_size);

	/* Get Report Descriptor */
	rpt_desc.size = desc_size;
	res = ioctl(fd, HIDIOCGRDESC, &rpt_desc);
	if (res < 0) 
	{
		perror("HIDIOCGRDESC");
	} else 
	{
		printf("Report Descriptor:\n");
		for (i = 0; i < rpt_desc.size; i++)
			printf("%hhx ", rpt_desc.value[i]);
		puts("\n");
	}

	/* Get Raw Name */
	res = ioctl(fd, HIDIOCGRAWNAME(256), buf);
	if (res < 0)
		perror("HIDIOCGRAWNAME");
	else
		printf("Raw Name: %s\n", buf);

	/* Get Physical Location */
	res = ioctl(fd, HIDIOCGRAWPHYS(256), buf);
	if (res < 0)
		perror("HIDIOCGRAWPHYS");
	else
		printf("Raw Phys: %s\n", buf);

	/* Get Raw Info */
	res = ioctl(fd, HIDIOCGRAWINFO, &info);
	if (res < 0) {
		perror("HIDIOCGRAWINFO");
	} else {
		printf("Raw Info:\n");
		printf("\tbustype: %d (%s)\n",
			info.bustype, bus_str(info.bustype));
		printf("\tvendor: 0x%04hx\n", info.vendor);
		printf("\tproduct: 0x%04hx\n", info.product);
	}
	show_menu();
	ch = getchar();
	while(1)
	{
		switch (ch) 
		{
			case 'a':
				ae_controle(fd);
				break;
			case 'i':
				iris_controle(fd);
				break;
			case 't':
				shutter_controle(fd);
				break;
			case 'g':
				gain_controle(fd);
				break;
			case 'n':
				nr_controle(fd);
				break;
			case 'w':
				wb_controle(fd);
				break;	
			case 'b':
				sat_controle(fd);
				break;	
			case 's':
				sharp_controle(fd);
				break;
			case 'x':
				zoomin_controle(fd);
				break;
			case 'y':
				zoomout_controle(fd);
				break;
			case 'z':
				zoomstop_controle(fd);
				break;
			case 'q':
				quit_flag=1;
				break;
			default: 
				error_opt = 1;
				printf("###############error_opt############\n");
				break;
		}
		if (quit_flag) 
		{
			quit_flag=0;
			printf("out\n");
			break;
		}
		//printf("###############1############\n");
		if (error_opt) 
		{
			error_opt=0;
			show_menu();
		}
		//printf("###############2############\n");
		//show_menu();
		printf("Pleae input the choice! \n");
		ch = getchar();
		sleep(1);
	};
	//printf("###############3############\n");

	return 0;
}

const char *bus_str(int bus)
{
	switch (bus) {
	case BUS_USB:
		return "USB";
		break;
	case BUS_HIL:
		return "HIL";
		break;
	case BUS_BLUETOOTH:
		return "Bluetooth";
		break;
	case BUS_VIRTUAL:
		return "Virtual";
		break;
	default:
		return "Other";
		break;
	}
}
