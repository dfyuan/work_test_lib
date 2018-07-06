#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <linux/input.h>
#include <errno.h>
#include "AitUVC.h"
#include "AitXU.hpp"

#define CHAR_BUFFERSIZE 20
typedef unsigned char BYTE;

using namespace std;

void CameraControl(AitXUHandle aitxu)
{
    char sel[CHAR_BUFFERSIZE];
    int Val = 0;
    int ret = 0;
   // CropParam crop_param;
    unsigned char cmd_in_16[16] = {0};
    unsigned char cmd_out_16[16] = {0};

    bool exit = false;


    while( !exit )
    {
        printf("\n\n");
        printf("[0] To Main Page.\n");
        printf("[2] Set Frame Rate\n");
	printf("[b] Set bitrate\n");
	printf("[i] Set I frame\n");
	
	printf("[m] Set streams ON/OFF\n");
	printf("[n] Set Crop pos\n");
	printf("[o] Get Crop pos\n");
	printf("[p] Get sensor resolution\n");
	printf("[q] Set Min fps\n");
	printf("[r] Get Lux\n");
	printf("[s] Get Color Temperature\n");
	printf("[t] Set System mode\n");
	printf("[u] Set shutter\n");
	printf("[v] Set gain\n");
	printf("[w] Set log\n");
        fgets( sel, CHAR_BUFFERSIZE, stdin );
        printf("\r\n");
        switch(sel[0])
        {
        case '0': //Set bitrate
            exit = true;
            break;
        case '2': //Set fps
            int Fps;
            printf("Fps (1~30)=");
            fgets( sel, CHAR_BUFFERSIZE, stdin );
            sscanf(sel,"%d", &Fps);
            AitXU_SetFrameRate(aitxu, Fps);
            break;
		case 'b': //Set bitrate
            int bitrate;
            printf("bitrate (200~4000)=");
            fgets( sel, CHAR_BUFFERSIZE, stdin );
            sscanf(sel,"%d", &bitrate);
            AitXU_SetBitrate(aitxu, bitrate);
            break;
		case 'i': //Set i frame
            AitXU_SetIFrame(aitxu);
            break;
		case 'g':
			int gop;
            printf("gop (10~50)=");
            fgets( sel, CHAR_BUFFERSIZE, stdin );
            sscanf(sel,"%d", &gop);
            AitXU_SetPFrameCount(aitxu, gop);
			break;
		case 'm':
			static unsigned char Stream_Number=0;
			static unsigned char Stream_Switch=0;
 
          printf("Enter stream number 1~4\r\n");  
          fgets( sel, CHAR_BUFFERSIZE, stdin );
          sscanf(sel, "%d", &Stream_Number);

          printf("Enter stream Switch [0]OFF [1]ON\r\n");  
          fgets( sel, CHAR_BUFFERSIZE, stdin );
          sscanf(sel, "%d", &Stream_Switch);

	  printf("Stream_Number=%d\r\n",Stream_Number);
	  printf("Stream_Switch=%d\r\n",Stream_Switch);

	  memset(cmd_in_16, 0 , 16);
	  memset(cmd_out_16, 0 , 16);
	  cmd_in_16[0] = 0x06;
	  cmd_in_16[1] = Stream_Number;
	  cmd_in_16[2] = Stream_Switch;

	  AitXU_Mmp16Cmd(aitxu,cmd_in_16,cmd_out_16);
	  if(!cmd_out_16[0])
		  printf("command ok \r\n");
	  else
		  printf("command fail \r\n");
	  break;
#if 0
        case 'n': /* set cropping control */
           memset(&crop_param,0,sizeof(CropParam));

            printf("[0]Disable,[1]Enable: \n");
            fgets( sel, CHAR_BUFFERSIZE, stdin );
            sscanf(sel, "%d", &Val);
            if(Val)
            {
                printf("StartX:\n");
                fgets( sel, CHAR_BUFFERSIZE, stdin );
                sscanf(sel, "%d", &Val);
                crop_param.cx = Val;

                printf("StartY:\n");
                fgets( sel, CHAR_BUFFERSIZE, stdin );
                sscanf(sel, "%d", &Val);
                crop_param.cy = Val;

                printf("Width:\n");
                fgets( sel, CHAR_BUFFERSIZE, stdin );
                sscanf(sel, "%d", &Val);
                crop_param.src_w = Val;

                printf("Height:\n");
                fgets( sel, CHAR_BUFFERSIZE, stdin );
                sscanf(sel, "%d", &Val);
                crop_param.src_h = Val;

                printf("Target W:\n");
                fgets( sel, CHAR_BUFFERSIZE, stdin );
                sscanf(sel, "%d", &Val);
                crop_param.dest_w = Val;

                printf("Target H:\n");
                fgets( sel, CHAR_BUFFERSIZE, stdin );
                sscanf(sel, "%d", &Val);
                crop_param.dest_h = Val;
            }
            ret = AitXU_SetCropControl(aitxu, crop_param.cx, crop_param.cy, crop_param.src_w, crop_param.src_h, crop_param.dest_w, crop_param.dest_h);
            if(ret < 0)
                printf("AIT CMD err!!!!\n");
            break;

        case 'o': /* get cropping control */
            ret = AitXU_GetCropControl(aitxu, &(crop_param.cx), &(crop_param.cy), &(crop_param.src_w), &(crop_param.src_h), &(crop_param.dest_w), &(crop_param.dest_h));
            if(ret < 0)
            {
                printf("AIT CMD err!!!!\n");
            }
            else
            {
                printf("StartX=%d\r\n", crop_param.cx);
                printf("StartY=%d\r\n", crop_param.cy);
                printf("Width=%d\r\n", crop_param.src_w);
                printf("Height=%d\r\n", crop_param.src_h);
                printf("TargetW=%d\r\n", crop_param.dest_w);
                printf("TargetH=%d\r\n", crop_param.dest_h);
            }
            break;
#endif
        case 'p': 
		static uint16_t sensor_w, sensor_h; 
		AitXU_Request_SensorResolution(aitxu);
		AitXU_Get_SensorResolution(aitxu, &sensor_w, &sensor_h);
                printf("Sensor_w=%d\r\n", sensor_w);
                printf("Sensor_h=%d\r\n", sensor_h);
            break;

        case 'q': 
          	static unsigned char Min_Fps=0; 
		 printf("Enter Min fsp:\r\n");  
		fgets( sel, CHAR_BUFFERSIZE, stdin );
          	sscanf(sel, "%d", &Min_Fps);
		AitXU_SetMinFps(aitxu, Min_Fps);
            break;

        case 'r': 
          	static uint16_t Lux=0; 
		AitXU_RequestLux(aitxu);
		AitXU_GetLux(aitxu,&Lux);
                printf(" Lux=%d\r\n",  Lux);
            break;

        case 's': 
            	static uint32_t Color_Temperature=0; 
            	memset(cmd_in_16, 0 , 16);
            	memset(cmd_out_16, 0 , 16);

            	cmd_in_16[0] = 0x29;
 		AitXU_Mmp16Cmd(aitxu,cmd_in_16,cmd_out_16);
	  	if(!cmd_out_16[0])
		{
			Color_Temperature = (cmd_out_16[1]<<24) | (cmd_out_16[2]<<16) | (cmd_out_16[3]<<8) | cmd_out_16[4];
			printf("Color_Temperature = %d\r\n",Color_Temperature);
		  	printf("command ok \r\n");
		}
	  	else
		  	printf("command fail \r\n");
	  
            break;

        case 't': 
             	static uint8_t System_Mode=0; 
            	memset(cmd_in_16, 0 , 16);
            	memset(cmd_out_16, 0 , 16);
		 printf("Enter System Mode[0~1]:\r\n");  
		fgets( sel, CHAR_BUFFERSIZE, stdin );
          	sscanf(sel, "%d", &System_Mode);
	
            	cmd_in_16[0] = 0x28;
		cmd_in_16[1] = System_Mode;
 		AitXU_Mmp16Cmd(aitxu,cmd_in_16,cmd_out_16);
	  	if(!cmd_out_16[0])
		{
		  	printf("command ok \r\n");
		}
	  	else
		  	printf("command fail \r\n");
            break;

        case 'u': 
          	static unsigned short Shutter_value=0; 
		 printf("Enter Shutter_value:\r\n");  
		fgets( sel, CHAR_BUFFERSIZE, stdin );
          	sscanf(sel, "%d", &Shutter_value);
		AitXU_SetShutter(aitxu, Shutter_value);
            break;

        case 'v': 
          	static unsigned short Gain_value=0; 
		 printf("Enter Gain_value:\r\n");  
		fgets( sel, CHAR_BUFFERSIZE, stdin );
          	sscanf(sel, "%d", &Gain_value);
		AitXU_SetGain(aitxu, Gain_value);
            break;

        case 'w': 
          	static unsigned char Log_value=0; 
		 printf("Enter Log_value[0:OFF  1:ON]:\r\n");  
		fgets( sel, CHAR_BUFFERSIZE, stdin );
          	sscanf(sel, "%d", &Log_value);
		AitXU_SetLog(aitxu, Log_value);
            break;

        }
	
    }
}

#include <dirent.h>
#include <poll.h>
#include <fcntl.h>
#define DEV_INPUT_EVENT "/dev/input"
#define AIT_UVC_CAMERA_INPUT_NAME "UVC Camera (04da:3911)"
#include <sys/stat.h>
#define AIT_UVC_CAMERA_VID "114d"

int OpenDevice(char *dev_name)
{
	struct stat st;

	if (-1 == stat(dev_name, &st)) {
		printf("Cannot identify '%s': %d, %s\n", dev_name, errno,
				strerror(errno));
	}

	if (!S_ISCHR(st.st_mode)) {
		printf("%s is no device\n", dev_name);
	}

	int m_Fd = open(dev_name, O_RDWR /* required */| O_NONBLOCK, 0);

	if (-1 == m_Fd) {
		printf("Cannot open '%s': %d, %s\n", dev_name, errno,
				strerror(errno));
	}
	return m_Fd;
}



int V4L2_IdentifyAitDevice(char *dev_name, char* VID,int check_vid)
{
	int dev = OpenDevice(dev_name);
	printf("open device %s = 0x%x\n",dev_name,dev);
	if(dev <= 0)
        {
            return -1;
        }

	extern void UVC_SetUVCKernelVersion(__u32 version);
	struct v4l2_capability capbility;
	       
	if( ioctl(dev,VIDIOC_QUERYCAP,&capbility)<0 )
		printf("VIDIOC_QUERYCAP error:\r\n");
	else
	{
		printf("V4L2 Capability:\r\n");
	        printf("driver		: %s\r\n",capbility.driver);
	        printf("card		: %s\r\n",capbility.card);
	        printf("bus_info	: %s\r\n",capbility.bus_info);
	        printf("version		: %u.%u.%u\r\n",(capbility.version&0xff0000)>>16,
               (capbility.version&0xff00)>>8,capbility.version&0xff);
	        printf("capabilities	: 0x%x\r\n",capbility.capabilities);
	      
	      UVC_SetUVCKernelVersion(capbility.version);
	}
	close(dev);

	for(int i = 0; i<sizeof(capbility.driver) ; i++)
	{

		if(capbility.driver[i] == 'u' &&
		   capbility.driver[i+1] == 'v' &&
		   capbility.driver[i+2] == 'c' &&	
	           capbility.driver[i+3] == 'v' &&
		   capbility.driver[i+4] == 'i' &&
		   capbility.driver[i+5] == 'd' &&
		   capbility.driver[i+6] == 'e' &&	
	           capbility.driver[i+7] == 'o')

		{
			printf("Get UVC video Device: %s\n",capbility.driver);
			break;
		}
	
		if (i == (sizeof(capbility.driver) -1) )
		{
			printf("Is not UVC video Device\n");
			return -1;
		}
			

	}

	if(check_vid)
	{
		for(int i = 0; i<sizeof(capbility.card) ; i++)
		{

			if(capbility.card[i] == *VID &&
		   	capbility.card[i+1] == *(VID+1) &&
		   	capbility.card[i+2] == *(VID+2) &&	
	           	capbility.card[i+3] == *(VID+3))
			{
				printf("Get AIT USB Device VID 0x%s\n",VID);
				break;
			}
	
			if (i == (sizeof(capbility.card) -1) )
			{
				printf("Is not AIT USB Device\n");
				return -1;
			}
			

		}
	}
 	
	return 0;

}

int main(int argc,char** argv)
{
	AitXUHandle aitxu = NULL;
	bool exit=false;
	char dev_name[15];
  	
    	
	sprintf(dev_name, "/dev/video%s", argv[1]);

	if(-1 == V4L2_IdentifyAitDevice(dev_name,"114d",0) )
		return -1;
		
	aitxu = AitXU_Init(dev_name);

    if(aitxu==0)
    {
		printf("AIT Device Not Found.\r\n");
		return 0;
    }

    while(!exit)
    {
	char sel[CHAR_BUFFERSIZE];
	printf("\n\n");
	printf("[0] Exit Program\n");
	printf("[3] Camera Control\n");
	printf("[c] Firmware Control\n");
	fgets( sel, CHAR_BUFFERSIZE, stdin );
	switch(sel[0])
	{
	    case '0':
		exit = true;
		break;

	    case '3':
		//CameraControl(aitxu);
		AitXU_WriteReg(aitxu,0x786,0x01);
		break;

	    case 'c':
		//FirmwareControl(aitxu);
		break;
	}
    }
	AitXU_Release(&aitxu);
	return 0;
}

