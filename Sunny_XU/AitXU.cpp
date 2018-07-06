#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <string.h>
#include <stdio.h>

#include <string.h>

#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <linux/videodev2.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <linux/types.h>
#include <stdint.h>
#include <linux/videodev2.h>

#include "UvcXU.hpp"
#include "AitXuDef.hpp"
#include "AitXU.hpp"
#include "video.h"

//Wrapper to pass non-fatal errors


int _PrintDebugging_(const char *format, ...)
{
    va_list vl;
    va_start(vl,format);
    printf("**AIT DEBUG**:: ");
    vprintf(format,vl);
#if 0 //output to file
    if(gDbgLog)
    {
        vfprintf(gDbgLog,format,vl);
        fflush(gDbgLog);
    }
#endif
    va_end(vl);
    return 0;
}

#define GUID_Skype_XU {0xBD,0x53,0x21,0xB4,0xD6,0x35,0xCA,0x45,0xB2,0x03,0x4E,0x01,0x49,0xB3,0x01,0xBC}
//#define GUID_Ait_XU1 {0xD0,0x9E,0xE4,0x23,0x78,0x11,0x31,0x4F,0xAE,0X52,0xD2,0xFB,0x8A,0x8D,0x3B,0x48}
#define GUID_Ait_XU1 {0x28,0xF0,0x33,0x70,0x63,0x11,0x4A,0x2E,0xBA,0X2C,0x68,0x90,0xEB,0x33,0x40,0x16}
//#define GUID_Ait_XU1 {0x23,0xe4,0x9E,0xd0,0x11,0x78,0x4f,0x31,0xAE,0X52,0xD2,0xFB,0x8A,0x8D,0x3B,0x48}
#if 1//!USE_UVCIOC_CTRL_QUERY
//#define EXU1_CS_SET_DATA 0x06
#define EXU1_CS_SET_DATA 0x01

static struct uvc_xu_control_info_2_6_32 aitxu_ctrls[] = {
  {
    GUID_Ait_XU1,       //{0xD0,0x9E,0xE4,0x23,0x78,0x11,0x31,0x4F,0xAE,0X52,0xD2,0xFB,0x8A,0x8D,0x3B,0x48},//GUID_Ait_XU1,   //GUID
    0,                  //index
    EXU1_CS_SET_DATA,    //control selector
    8*4,                  //size
    UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_DEF //info
  }
};

#else
static struct uvc_xu_control_info_2_6_32 aitxu_ctrls[] = {
  {
    GUID_Ait_XU1,       //{0xD0,0x9E,0xE4,0x23,0x78,0x11,0x31,0x4F,0xAE,0X52,0xD2,0xFB,0x8A,0x8D,0x3B,0x48},//GUID_Ait_XU1,   //GUID
    0,                  //index
    EXU1_CS_SET_ISP,    //control selector
    8,                  //size
    UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_DEF //info
  },
  {
    GUID_Ait_XU1,           //GUID
    1,                      //index
    EXU1_CS_GET_ISP_RESULT, //control selector
    8,                      //size
    UVC_CONTROL_GET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_DEF //info
  },

  {
    GUID_Ait_XU1,           //GUID
    2,                      //index
    EXU_CS_SET_MMP_CMD16,   //control selector
    16,                     //size
    UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_DEF //info
  },
  {
    GUID_Ait_XU1,           //GUID
    3,                      //index
    EXU_CS_GET_MMP_RESULT16,//control selector ,GET CMD 16
    16,                     //size
    UVC_CONTROL_GET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_DEF //info
  },

  {
    GUID_Ait_XU1,       //{0xD0,0x9E,0xE4,0x23,0x78,0x11,0x31,0x4F,0xAE,0X52,0xD2,0xFB,0x8A,0x8D,0x3B,0x48},//GUID_Ait_XU1,   //GUID
    4,                  //index
    EXU1_CS_SET_MMP,    //control selector
    8,                  //size
    UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_DEF //info
  },
  {
    GUID_Ait_XU1,           //GUID
    5,                      //index
    EXU1_CS_GET_MMP_RESULT, //control selector
    8,                      //size
    UVC_CONTROL_GET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_DEF //info
  },
  {
    GUID_Ait_XU1,           //GUID
    6,                      //index
    EXU1_CS_GET_DATA,       //control selector
    EXU1_CS_GET_DATA_LEN,   //size
    UVC_CONTROL_GET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_DEF //info
  },
  {
    GUID_Ait_XU1,           //GUID
    7,                      //index
    EXU1_CS_SET_FW_DATA,       //control selector
    32,   //size
    UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_DEF //info
  },
  {
    GUID_Ait_XU1,           //GUID
    8,                      //index
    EXU1_CS_SET_ISP_EX,         //control selector
    16,                     //size
    UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_DEF //info
  },
  {
    GUID_Ait_XU1,           //GUID
    9,                      //index
    EXU1_CS_GET_ISP_EX_RESULT,//control selector ,GET CMD 16
    16,                     //size
    UVC_CONTROL_GET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_DEF //info
  },
  {
    GUID_Ait_XU1,           //GUID
    10,                      //index
    EXU1_CS_READ_MMP_MEM,//control selector ,GET CMD 16
    16,                     //size
    UVC_CONTROL_GET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_DEF //info
  },
  {
    GUID_Ait_XU1,           //GUID
    11,                      //index
    EXU1_CS_WRITE_MMP_MEM,//control selector ,GET CMD 16
    16,                     //size
    UVC_CONTROL_GET_CUR | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX | UVC_CONTROL_GET_DEF //info
  }

};
#endif //end of !USE_UVCIOC_CTRL_QUERY
#if 1
#define V4L_CID_AIT_XU_SET_DATA 1
#if 0
static struct uvc_xu_control_mapping_2_6_32 aitxu_mappings_2_6_32[] =
{
	{
		V4L_CID_AIT_XU_SET_DATA,         //id
		"Set ASIC",                      //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_SET_DATA,                //control selector
		8*4,                            //size
		0,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	}
};
#else
static struct uvc_xu_control_mapping_3_2_0  aitxu_mapping_3_2_0[] =
{
	{
		V4L_CID_AIT_XU_SET_DATA,         //id
		"Set ASIC",                      //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_SET_DATA,                //control selector
		8*4,                            //size
		0,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	}
};

#endif
#else

static struct uvc_xu_control_mapping_2_6_32 aitxu_mappings_2_6_32[] =
 
{
	{
		V4L_CID_AIT_XU_SET_ISP,         //id
		"Set ISP",                      //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_SET_ISP,                //control selector
		8*8,                            //size
		0,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	},
	{
		V4L_CID_AIT_XU_GET_ISP_RESULT,  //id
		"Get ISP",                      //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_GET_ISP_RESULT,         //control selector
		8*8,                            //size
		0,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	},
	{
		V4L_CID_AIT_XU_SET_MMP,         //id
		"Set MMP",                      //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_SET_MMP,                //control selector
		8*8,                            //size
		0,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	},
	{
		V4L_CID_AIT_XU_GET_MMP_RESULT,  //id
		"Get MMP",                      //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_GET_MMP_RESULT,         //control selector
		8*8,                            //size
		0,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	},
	{
		V4L_CID_AIT_XU_SET_MMP_CMD16,   //id
		"Set MMP16",                    //name
		GUID_Ait_XU1,                   //entity
		EXU_CS_SET_MMP_CMD16,           //control selector
		16*8,                            //size
		0,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	},
	{
		V4L_CID_AIT_XU_GET_MMP_CMD16_RESULT, //id
		"Get MMP16 Result",             //name
		GUID_Ait_XU1,                   //entity
		EXU_CS_GET_MMP_RESULT16,        //control selector
		16*8,                           //size
		0,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	},

	{
		V4L_CID_AIT_XU_GET_DATA,
		"AIT XU Get Data",
		GUID_Ait_XU1,
		EXU1_CS_GET_DATA,
		EXU1_CS_GET_DATA_LEN*8,
		0,
		V4L2_CTRL_TYPE_INTEGER,
		UVC_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		V4L_CID_AIT_XU_SET_FW_DATA,
		"AIT XU Set FW Data",
		GUID_Ait_XU1,
		EXU1_CS_SET_FW_DATA,
		32*8,
		0,
		V4L2_CTRL_TYPE_INTEGER,
		UVC_CTRL_DATA_TYPE_UNSIGNED,
	},
	{
		V4L_CID_AIT_XU_SET_ISP_EX,      //id
		"Set ISP EX",                    //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_SET_ISP_EX,           //control selector
		16*8,                            //size
		0,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	},
	{
		V4L_CID_AIT_XU_GET_ISP_EX_RESULT, //id
		"Get ISP EX Result",             //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_GET_ISP_EX_RESULT,        //control selector
		16*8,                           //size
		0,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	},
	{
		V4L_CID_AIT_XU_SET_MMP_MEM, //id
		"Get MMP MEM",             //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_READ_MMP_MEM,        //control selector
		16*8,                           //size
		0,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		UVC_CTRL_DATA_TYPE_SIGNED      //data_type
	},

	{
		V4L_CID_AIT_XU_GET_MMP_MEM, //id
		"Set MMP MEM",             //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_WRITE_MMP_MEM,        //control selector
		16*8,                           //size
		0,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		UVC_CTRL_DATA_TYPE_SIGNED      //data_type
	},
	{
		V4L_CID_AIT_XU_SET_CROP_PARAM7,   //id ,, To meet Eyesight's requirement, change first param to V4L_CID_AIT_XU_SET_CROP_PARAM7
		"CROP PARAM1",                    //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_CROP_IMAGE,             //control selector
		//16*8,                            //size
		2*8,
		0,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	},

	{
		V4L_CID_AIT_XU_SET_CROP_PARAM0,   //id
		"CROP PARAM2",                    //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_CROP_IMAGE,             //control selector
		//16*8,                            //size
		2*8,
		2*8,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	},

	{
		V4L_CID_AIT_XU_SET_CROP_PARAM1,   //id
		"CROP PARAM3",                    //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_CROP_IMAGE,             //control selector
		//16*8,                            //size
		2*8,
		4*8,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	},

	{
		V4L_CID_AIT_XU_SET_CROP_PARAM2,   //id
		"CROP PARAM4",                    //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_CROP_IMAGE,             //control selector
		//16*8,                            //size
		2*8,
		6*8,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	},

	{
		V4L_CID_AIT_XU_SET_CROP_PARAM3,   //id
		"CROP PARAM5",                    //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_CROP_IMAGE,             //control selector
		//16*8,                            //size
		2*8,
		8*8,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		//V4L2_CTRL_TYPE_INTEGER64,
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	},

	{
		V4L_CID_AIT_XU_SET_CROP_PARAM4,   //id
		"CROP PARAM6",                    //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_CROP_IMAGE,             //control selector
		//16*8,                            //size
		2*8,
		10*8,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		//V4L2_CTRL_TYPE_INTEGER64,
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	},
	{
		V4L_CID_AIT_XU_SET_CROP_PARAM5,   //id
		"CROP PARAM7",                    //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_CROP_IMAGE,             //control selector
		//16*8,                            //size
		2*8,
		12*8,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		//V4L2_CTRL_TYPE_INTEGER64,
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	},
	{
		V4L_CID_AIT_XU_SET_CROP_PARAM6,   //id
		"CROP PARAM8",                    //name
		GUID_Ait_XU1,                   //entity
		EXU1_CS_CROP_IMAGE,             //control selector
		//16*8,                            //size
		2*8,
		14*8,                              //offset
		V4L2_CTRL_TYPE_INTEGER,         //v4l2_type
		//V4L2_CTRL_TYPE_INTEGER64,
		UVC_CTRL_DATA_TYPE_SIGNED,      //data_type
	},
};
#endif
typedef struct _AitXU
{
    int dev;
    //bool ForceKeyFrame;
    //int yuv_w;
    //int yuv_h;
}AitXU;

//General Commands


AitXUHandle AitXU_Init(char *dev_name)
{
    DbgMsg("AitXU_Init\r\n");
    
    int dev = open(dev_name, O_RDWR | O_NONBLOCK, 0);

    if(dev <= 0)
    {
        DbgMsg("AitXU Lib: Video device %s not exist.\r\n",dev_name);
        return 0;
    }

    DbgMsg("AitXU_Init: open devive %s. \r\n",dev_name);

    return AitXU_Init_from_handle(dev);
}

//initialize from device handle
AitXUHandle AitXU_Init_from_handle(int dev)
{
    extern __u32 UVC_GetUVCKernelVersion(void);
   // DbgMsg("AitXU_Init\r\n");

    AitXU *handle = 0;
    int err =0;
        int ret;
    handle = new AitXU;
    handle->dev = dev;

	#if 1
    if(UVC_GetUVCKernelVersion()<=KERNEL_VERSION(3,0,0))
    {
		DbgMsg("AitXU_Init: Add XU controls. \r\n");

		for(unsigned int n=0; n<sizeof(aitxu_ctrls)/sizeof(uvc_xu_control_info_2_6_32); ++n)
		{
		    if ( (err = ioctl(handle->dev, UVCIOC_CTRL_ADD_2_6_32, &aitxu_ctrls[n])) < 0 )
		    {
		      if ( errno != EEXIST )
		      {
				DbgMsg("AitXU_Init: ADD_CTRL(%d) failed.\r\n",aitxu_ctrls[n].index);
				DbgMsg("errno=%d, %s\r\n",errno,strerror(errno));
		      }
		      if(errno == EACCES || errno == 1)
		      {
				//need root previledges
				  DbgMsg("AitXU_Init: need root previledges. \r\n");
				  DbgMsg("errno=%d, %s\r\n",errno,strerror(errno));
				  return NULL;
		      }
		    }
		}
    }


    //mapping xp ID
    DbgMsg("AitXU_Init: mapping XU controls. \r\n");
   // for(unsigned int n=0;n<sizeof(aitxu_mappings_2_6_32)/sizeof(uvc_xu_control_mapping_2_6_32);++n)
   for(unsigned int n=0;n<sizeof(aitxu_mapping_3_2_0)/sizeof(uvc_xu_control_mapping_3_2_0);++n)
    {
        //struct uvc_xu_control_mapping_2_6_36 new_mapping;
        struct uvc_xu_control_mapping_3_2_0 new_mapping;

        errno=0;

		new_mapping.id = aitxu_mapping_3_2_0[n].id;
		memcpy((char*)new_mapping.name,(char*)aitxu_mapping_3_2_0[n].name,32);
		memcpy((char*)new_mapping.entity,(char*)aitxu_mapping_3_2_0[n].entity,16);
		new_mapping.selector = aitxu_mapping_3_2_0[n].selector;        
		new_mapping.size= aitxu_mapping_3_2_0[n].size;        
		new_mapping.offset = aitxu_mapping_3_2_0[n].offset;        
		new_mapping.v4l2_type = aitxu_mapping_3_2_0[n].v4l2_type;        
		new_mapping.data_type = aitxu_mapping_3_2_0[n].data_type;     
		printf("Add XU: %s  \n",(char*)aitxu_mapping_3_2_0[n].name);

		ret = ioctl( handle->dev, UVCIOC_CTRL_MAP_3_2_0, &new_mapping);//aitxu_mappings_2_6_32[n] );	
		if( ret != 0  && errno != EEXIST ) 
		{
			  //debug_printf
			  
		    err = ioctl(handle->dev, UVCIOC_CTRL_MAP_3_2_0, &aitxu_mapping_3_2_0[n]);
		    if( ret != 0  && errno != EEXIST ) {
			printf( "XU Mapping err  = %d\r\n",err );
		    } else if( errno == EEXIST ) {
			ret = 0; //This error is OK
			printf( "Exist!\r\n" );
			continue;
		    }else if(errno == EACCES || errno == 1)
		    {
			//need root previledges
			DbgMsg("AitXU_Init: need root privileges. \r\n");
			DbgMsg("errno=%d, %s\r\n",errno,strerror(errno));
			return NULL;
		    }
		}

		if( ret != 0  && errno != EEXIST )
		{
		    printf( "XU Mapping err  = %d\r\n",err );
		} 
		else if( errno == EEXIST )
		{
		    ret = 0; //This error is OK
		    printf( "Exist!\r\n" );
		}else if(errno == EACCES || errno == 1)
		{
		    //need root previledges
		    DbgMsg("AitXU_Init: need root privileges. \r\n");
		    DbgMsg("errno=%d, %s\r\n",errno,strerror(errno));
		    return NULL;
		}
    }
	#endif
    DbgMsg("AitXU_Init: OK. \r\n");

    return (AitXUHandle)handle;
}

void AitXU_Release(AitXUHandle* handle)
{
    if(handle)
    {
        AitXU *ait_xu = (AitXU*)(*handle);

    	if(ait_xu->dev >= 0)
       	{
	 	if (-1 == close(ait_xu->dev))
			DbgMsg("AitXU_Release:: failed to close device. \r\n");
	  	else
			DbgMsg("AitXU_Release:: close device. \r\n");

	}

        delete ait_xu;
        *handle = NULL;
    }

}

void AitXU_Release2(AitXUHandle* handle)	//special release function for SkypeXU_Init
{
    if(handle)
    {
        AitXU *ait_xu = (AitXU*)(*handle);
        delete ait_xu;
        *handle = NULL;
    }
}

int AitXU_IspCmd(AitXUHandle handle,uint8_t *cmd_in,uint8_t *cmd_out)
{
//    DbgMsg("AitXU_IspCmd\r\n");
    //AitXU *aitxu = (AitXU*)handle;

    if(cmd_in)
    {
        if( AitXU_XuCmd(handle,cmd_in,EXU1_CS_SET_ISP,8,AIT_XU_CMD_OUT)<0 )
        {
            DbgMsg("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }
    }

    if(cmd_out)
    {
        //get result from device
    }

    return 0;
}

int AitXU_Mmp16Cmd(AitXUHandle handle,uint8_t *cmd_in,uint8_t *cmd_out)
{
    #ifdef DEBUG_MSG
    printf("AitXU_Mmp16Cmd\r\n");
    #endif

    if(cmd_in)
    {
        if( AitXU_XuCmd(handle,cmd_in,EXU1_CS_SET_MMP_CMD16,16,AIT_XU_CMD_OUT)<0 )
        {
            printf("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }
    }

    if(cmd_out)
    {
        //get result from device
        if( AitXU_XuCmd(handle,cmd_out,EXU1_CS_GET_MMP_RESULT16,16,AIT_XU_CMD_IN)<0 )
        {
            printf("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }
    }

    return 0;
}

int AitXU_SetShutter(AitXUHandle handle,uint16_t Shutter)
{
    #ifdef DEBUG_MSG
    printf("AitXU_SetShutter\r\n");
    #endif

	uint8_t cmd_in[16]={0};
	cmd_in[0]=0x05;
	cmd_in[1]=Shutter>>8;
	cmd_in[2]=Shutter;

	printf("EXU1_CS_CROP_IMAGE=%d\r\n",EXU1_CS_CROP_IMAGE);
        if( AitXU_XuCmd(handle,cmd_in,EXU1_CS_CROP_IMAGE,16,AIT_XU_CMD_OUT)<0 )
        {
            printf("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }
    
    return 0;
}

int AitXU_SetGain(AitXUHandle handle,uint16_t Gain)
{
    #ifdef DEBUG_MSG
    printf("AitXU_SetGain\r\n");
    #endif

	uint8_t cmd_in[16]={0};
	cmd_in[0]=0x06;
	cmd_in[1]=Gain>>8;
	cmd_in[2]=Gain;

	printf("EXU1_CS_CROP_IMAGE=%d\r\n",EXU1_CS_CROP_IMAGE);
        if( AitXU_XuCmd(handle,cmd_in,EXU1_CS_CROP_IMAGE,16,AIT_XU_CMD_OUT)<0 )
        {
            printf("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }
    
    return 0;
}

int AitXU_SetLog(AitXUHandle handle,uint8_t Log)
{
    #ifdef DEBUG_MSG
    printf("AitXU_SetLog\r\n");
    #endif

	uint8_t cmd_in[16]={0};
	cmd_in[0]=0x07;
	cmd_in[1]=Log;

	printf("EXU1_CS_CROP_IMAGE=%d\r\n",EXU1_CS_CROP_IMAGE);
        if( AitXU_XuCmd(handle,cmd_in,EXU1_CS_CROP_IMAGE,16,AIT_XU_CMD_OUT)<0 )
        {
            printf("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }
    
    return 0;
}


int AitXU_SetMinFps(AitXUHandle handle,uint8_t Min_Fps)
{
    #ifdef DEBUG_MSG
    printf("AitXU_SetMinFps\r\n");
    #endif

	uint8_t cmd_in[16]={0};
	cmd_in[0]=0x04;
	cmd_in[1]=Min_Fps;

	printf("EXU1_CS_CROP_IMAGE=%d\r\n",EXU1_CS_CROP_IMAGE);
        if( AitXU_XuCmd(handle,cmd_in,EXU1_CS_CROP_IMAGE,16,AIT_XU_CMD_OUT)<0 )
        {
            printf("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }
    
    return 0;
}

int AitXU_RequestLux(AitXUHandle handle)
{
    #ifdef DEBUG_MSG
    printf("AitXU_RequestLux\r\n");
    #endif

	uint8_t cmd_in[16]={0};
	cmd_in[0]=0x03;
	printf("EXU1_CS_CROP_IMAGE=%d\r\n",EXU1_CS_CROP_IMAGE);
        if( AitXU_XuCmd(handle,cmd_in,EXU1_CS_CROP_IMAGE,16,AIT_XU_CMD_OUT)<0 )
        {
            printf("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }
    
    return 0;
}

int AitXU_GetLux(AitXUHandle handle,uint16_t* Lux)
{
    #ifdef DEBUG_MSG
    printf("AitXU_GetLux\r\n");
    #endif

	uint8_t cmd_out[16]={0};

        //get result from device
        if( AitXU_XuCmd(handle,cmd_out,EXU1_CS_CROP_IMAGE,16,AIT_XU_CMD_IN)<0 )
        {
            printf("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }

	*Lux = cmd_out[0]  + (cmd_out[1]<<8);


    return 0;

}

int AitXU_Request_SensorResolution(AitXUHandle handle)
{
    #ifdef DEBUG_MSG
    printf("AitXU_Request_SensorResolution\r\n");
    #endif

	uint8_t cmd_in[16]={0};
	cmd_in[0]=0x02;
	printf("EXU1_CS_CROP_IMAGE=%d\r\n",EXU1_CS_CROP_IMAGE);
        if( AitXU_XuCmd(handle,cmd_in,EXU1_CS_CROP_IMAGE,16,AIT_XU_CMD_OUT)<0 )
        {
            printf("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }
    
    return 0;
}

int AitXU_Get_SensorResolution(  AitXUHandle handle
                        , uint16_t *sensor_w, uint16_t *sensor_h)
{
    #ifdef DEBUG_MSG
    printf("AitXU_Get_SensorResolution\r\n");
    #endif

	uint8_t cmd_out[16]={0};

        //get result from device
        if( AitXU_XuCmd(handle,cmd_out,EXU1_CS_CROP_IMAGE,16,AIT_XU_CMD_IN)<0 )
        {
            printf("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }

	*sensor_w = cmd_out[0]  + (cmd_out[1]<<8);
	*sensor_h = cmd_out[2]  + (cmd_out[3]<<8);

    return 0;
}

int AitXU_SetCropControl(   AitXUHandle handle
                        ,   uint16_t start_x, uint16_t start_y
                        ,   uint16_t src_w, uint16_t src_h
                        ,   uint16_t dest_w, uint16_t dest_h)
{
    #ifdef DEBUG_MSG
    printf("AitXU_SetCropControl\r\n");
    #endif

	uint8_t cmd_in[16]={0};
	cmd_in[0]=0x01;
	cmd_in[1]=0x00;
	cmd_in[2]=start_x;
	cmd_in[3]=start_x>>8;
	cmd_in[4]=start_y;
	cmd_in[5]=start_y>>8;
	cmd_in[6]=src_w;
	cmd_in[7]=src_w>>8;
	cmd_in[8]=src_h;
	cmd_in[9]=src_h>>8;
	cmd_in[10]=dest_w;
	cmd_in[11]=dest_w>>8;
	cmd_in[12]=dest_h;
	cmd_in[13]=dest_h>>8;
	cmd_in[14]=0x00;
	cmd_in[15]=0x00;

        if( AitXU_XuCmd(handle,cmd_in,EXU1_CS_CROP_IMAGE,16,AIT_XU_CMD_OUT)<0 )
        {
            printf("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }
    
    return 0;
}

int AitXU_GetCropControl(  AitXUHandle handle
                        ,   uint16_t *start_x, uint16_t *start_y
                        ,   uint16_t *src_w, uint16_t *src_h
                        ,   uint16_t *dest_w, uint16_t *dest_h)
{
    #ifdef DEBUG_MSG
    printf("AitXU_GetCropControl\r\n");
    #endif

	uint8_t cmd_out[16]={0};

        //get result from device
        if( AitXU_XuCmd(handle,cmd_out,EXU1_CS_CROP_IMAGE,16,AIT_XU_CMD_IN)<0 )
        {
            printf("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }

	*start_x = cmd_out[2]  + (cmd_out[3]<<8);
	*start_y = cmd_out[4]  + (cmd_out[5]<<8);
	*src_w   = cmd_out[6]  + (cmd_out[7]<<8);
	*src_h   = cmd_out[8]  + (cmd_out[9]<<8);
	*dest_w  = cmd_out[10] + (cmd_out[11]<<8);
	*dest_h  = cmd_out[12] + (cmd_out[13]<<8);

    return 0;
}

int AitXU_MmpCmd(AitXUHandle handle,uint8_t *cmd_in,uint8_t *cmd_out)
{
    DbgMsg("AitXU_MmpCmd\r\n");

    //AitXU *aitxu = (AitXU*)handle;

    if(cmd_in)
    {
        if( AitXU_XuCmd(handle, cmd_in, EXU1_CS_SET_MMP, 8, AIT_XU_CMD_OUT)<0 )
        {
            DbgMsg("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }
    }

    if(cmd_out)
    {
        //get result from device
        if( AitXU_XuCmd(handle ,cmd_out, EXU1_CS_GET_MMP_RESULT, 8, AIT_XU_CMD_IN)<0 )
        {
            DbgMsg("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }
    }

    return 0;
}



int AitXU_MmpMemCmd(AitXUHandle handle,uint8_t *cmd_in,uint8_t *cmd_out)
{
    DbgMsg("AitXU_MmpMemCmd\r\n");

   if(cmd_in)
    {
        int ret = AitXU_XuCmd(handle,cmd_in,EXU1_CS_WRITE_MMP_MEM,16,AIT_XU_CMD_OUT);
        printf("ret =%d\n",ret);
        if( ret <0)
        {
            DbgMsg("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }
    }

    if(cmd_out)
    {
        //get result from device
        if( AitXU_XuCmd(handle,cmd_out,EXU1_CS_READ_MMP_MEM,16,AIT_XU_CMD_IN)<0 )
        {
            DbgMsg("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }
    }


    return 0;
}

int AitXU_SetSelectStreamID4Setting(AitXUHandle handle,int id)
{
    DbgMsg("AitXU_SetBitrate\r\n");

    int err=0;

    __u8 CmdIn[8] = {0x13,0x00,0,0,0,0,0,0};
    
    if(id)
      CmdIn[1] = id;
    
    if( AitXU_IspCmd(handle,CmdIn,NULL) != 0)
    {
        DbgMsg("uvcioc ctrl set error: errno=%d (retval=%d)", errno, err);
        return AIT_XU_ERROR;
    }
    return AIT_XU_OK;
}


int AitXU_SetEncRes(AitXUHandle handle,uint8_t resolution)
{
    DbgMsg("SkypeXU_SetEncRes\r\n");

    errno = 0;
    unsigned char cmd[8]={0};
    cmd[0] = EXU_MMP_ENCODING_RESOLUTION;
    cmd[1] = resolution;


    if( AitXU_XuCmd(handle,cmd,EXU1_CS_SET_MMP,8,AIT_XU_CMD_OUT)<0 )
    {
        //printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! FAILED TO SET RESOLUTION (ERROR NO =%d)!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n",errno);
        return SKYPE_XU_ERROR;
    }


    return AIT_XU_OK;
}


int AitXU_SetBitrate(AitXUHandle handle,int bitrate)
{
    DbgMsg("AitXU_SetBitrate\r\n");

    int err=0;
    if (bitrate > 0xFFFF) {
        DbgMsg("bitrate out of range");
        return AIT_XU_OUT_OF_RANGE;
    }

    __u8 CmdIn[8] = {8,0xFF,0,0,0,0,0,0};
    CmdIn[2] = bitrate&0xFF;
    CmdIn[3] = (bitrate>>8)&0Xff;
    if( AitXU_IspCmd(handle,CmdIn,NULL) != 0)
    {
        DbgMsg("uvcioc ctrl set error: errno=%d (retval=%d)", errno, err);
        return AIT_XU_ERROR;
    }
    return AIT_XU_OK;
}

int AitXU_SetFrameRate(void* handle,uint8_t rate)
{
    DbgMsg("AitXU_SetFrameRate\r\n");

    int err=0x80000000;
    uint8_t new_rate = 0;

    if (rate < 5) {
        //new_rate = 5;
    }
    else if (rate > 30) {
        new_rate = 30;
    }
    else {
        new_rate = rate;
    }

    __u8 CmdIn[8] = {0x02,0x00,0,0,0,0,0,0};
    CmdIn[1] = new_rate&0xFF;
    CmdIn[2] = (new_rate>>8)&0xff;

    err = AitXU_IspCmd(handle,CmdIn,NULL);
    if(err<0)
    {
        DbgMsg("uvcioc ctrl set error: errno=%d (retval=%d)", errno, err);
        return AIT_XU_ERROR;
    }

    return AIT_XU_OK;
}

int AitXU_SetIFrame(AitXUHandle handle)
{
    DbgMsg("AitXU_SetIFrame\r\n");

    int err=0x80000000;
    __u8 CmdIn[8] = {0x04,0x00,0,0,0,0,0,0};

    err = AitXU_IspCmd(handle,CmdIn,NULL);
    if(err<0)
    {
        DbgMsg("failed force I frame.\r\n");
        DbgMsg("uvcioc ctrl set error: errno=%d (retval=%d)", errno, err);
        return AIT_XU_ERROR;
    }
    DbgMsg("AitXU send force I.\r\n");

    return AIT_XU_OK;
}

int AitXU_SetPFrameCount(AitXUHandle handle,unsigned int val)
{
    int err=0x80000000;
    unsigned char cmd[8] = {0};

    cmd[0] = EXU_MMP_SET_P_COUNT;
    cmd[1] = val&0xFF;
    cmd[2] = (val>>8)&0xFF;
    cmd[3] = (val>>16)&0xFF;
    cmd[4] = (val>>24)&0xFF;

    err = AitXU_XuCmd(handle, cmd, EXU1_CS_SET_MMP,8,1);
    if(err<0)
    {
        //ERROR("failed force I frame.\r\n");
        //ERROR("uvcioc ctrl set error: errno=%d (retval=%d)", errno, err);
        return AIT_XU_ERROR;
    }
    return AIT_XU_OK;
}

int AitXU_XuCmd(AitXUHandle handle,unsigned char* cmd,unsigned short cs,unsigned char len, unsigned char direction)
{

    extern int UVC_XuCmd_V2(int handle,unsigned char* cmd,unsigned short cs,unsigned char len, unsigned int query, unsigned char unit);
    AitXU *aitxu = (AitXU*)handle;

    return UVC_XuCmd_V2(aitxu->dev,	cmd,
                        cs,
                        len,
                        (direction==AIT_XU_CMD_IN)?UVC_GET_CUR:UVC_SET_CUR,   //query
                        XU_UNIT_ID_AIT_XU);
}

int AitXU_WriteSensorReg(AitXUHandle handle,unsigned short addr, unsigned short val)
{
    unsigned char cmd[8];
    cmd[0] = EXUID_SET_REG;
    cmd[1] = SENSOR_WRITE;
    cmd[2] = addr & 0xff;
    cmd[3] = addr>>8 & 0xff;
    cmd[4] = val & 0xff;
    cmd[5] = val>>8 & 0xff;
    cmd[6] = 0;
    cmd[7] = 0;
    return AitXU_XuCmd(handle,cmd,EXU1_CS_SET_ISP,8,AIT_XU_CMD_OUT);
}

int AitXU_WriteReg(AitXUHandle handle,unsigned short addr, unsigned char val)
{
    unsigned char cmd[4];
   /* cmd[0] = EXUID_SET_REG;
    cmd[1] = REG_WRITE;
    cmd[2] = addr & 0xff;
    cmd[3] = addr>>8 & 0xff;*/
    cmd[0] = 0x07;
    cmd[1] = 0x86;
    cmd[2] = 0x03;
    cmd[3] = 0x00;
   // cmd[4] = val & 0xff;

    //return AitXU_XuCmd(handle,cmd,EXU1_CS_SET_ISP,8,AIT_XU_CMD_OUT);
return AitXU_XuCmd(handle,cmd,0,4,AIT_XU_CMD_OUT);
}

int AitXU_ReadReg(AitXUHandle handle,unsigned short addr, unsigned char *val)
{
    int hr=0;
    unsigned char cmd[4];
    cmd[0] = 0x07;
    cmd[1] = 0x86;
    cmd[2] = 0;
    cmd[3] = 0x00;
 

    //hr = AitXU_XuCmd(handle,cmd,EXU1_CS_SET_ISP,8,AIT_XU_CMD_OUT);
hr = AitXU_XuCmd(handle,cmd,6,4,AIT_XU_CMD_OUT);
    if(hr<0)
        return hr;

    memset(cmd,0,8);
    hr = AitXU_XuCmd(handle,cmd,6,4,AIT_XU_CMD_IN);
    //hr = AitXU_XuCmd(handle,cmd,EXU1_CS_GET_ISP_RESULT,8,AIT_XU_CMD_IN);
    *val = cmd[2];// + cmd[3]*0x100;
//printf(¨>>>>>>>>>>>>>read REG %#x¨,val);
    return SUCCESS;

}

int AitXU_ReadSensorReg(AitXUHandle handle,unsigned short addr, unsigned short *val)
{
    int hr=0;
    unsigned char cmd[8];
    cmd[0] = EXUID_SET_REG;
    cmd[1] = SENSOR_READ;
    cmd[2] = addr & 0xff;
    cmd[3] = addr>>8 & 0xff;
    cmd[4] = 0;
    cmd[5] = 0;
    cmd[6] = 0;
    cmd[7] = 0;

    hr = AitXU_XuCmd(handle,cmd,EXU1_CS_SET_ISP,8,AIT_XU_CMD_OUT);
    if(hr<0)
        return hr;

    memset(cmd,0,8);
    hr = AitXU_XuCmd(handle,cmd,EXU1_CS_GET_ISP_RESULT,8,AIT_XU_CMD_IN);
    *val = cmd[2] + cmd[3]*0x100;

    return SUCCESS;

}

int AitXU_ReadCodecReg(AitXUHandle handle, unsigned short addr, unsigned char *val)
{
    int hr=0;
    unsigned char cmd[8] = {0};
    cmd[0] = 0xFF;  //mmp sub command
    cmd[1] = 0x00;  //sub command set i2c address
    cmd[2] = 0x14;  //codec i2c address

    //set I2C addr
    hr = AitXU_XuCmd(handle,cmd,EXU1_CS_SET_MMP,8,AIT_XU_CMD_OUT);
    if(hr<0)
        return hr;

    //set read address
    cmd[0] = 0xFF;  //mmp sub command
    cmd[1] = 0x02;  //set i2c address
    cmd[2] = (addr>>8)&0xFF;    //address low byte
    cmd[3] = addr&0xFF;         //address hight byte
    hr = AitXU_XuCmd(handle,cmd,EXU1_CS_SET_MMP,8,AIT_XU_CMD_OUT);
    if(hr<0)
        return hr;

    //get value
    cmd[0] = 0;
    cmd[1] = 0;
    cmd[2] = 0;
    cmd[3] = 0;
    hr = AitXU_XuCmd(handle,cmd,EXU1_CS_GET_MMP_RESULT,8,AIT_XU_CMD_IN);
    if(hr<0)
        return hr;

    *val = cmd[2];

    return SUCCESS;
}

int AitXU_WriteCodecReg(AitXUHandle handle, unsigned short addr, unsigned char val)
{
    int hr=0;
    unsigned char cmd[8] = {0};
    cmd[0] = 0xFF;  //mmp sub command
    cmd[1] = 0x00;  //set i2c address
    cmd[2] = 0x14;  //codec i2c address

    //set I2C addr
    hr = AitXU_XuCmd(handle,cmd,EXU1_CS_SET_MMP,8,AIT_XU_CMD_OUT);
    if(hr<0)
        return hr;

    //write register
    cmd[0] = 0xFF;  //mmp sub command
    cmd[1] = 0x01;  //set i2c address
    cmd[2] = (addr>>8)&0xFF;    //address low byte
    cmd[3] = addr&0xFF;         //address hight byte
    cmd[4] = val;

    hr = AitXU_XuCmd(handle,cmd,EXU1_CS_SET_MMP,8,AIT_XU_CMD_OUT);
    if(hr<0)
        return hr;

    return SUCCESS;
}

//For SEC commands

int AitXU_GetVideoOption(AitXUHandle handle,unsigned int *options)
{
    int hr=0;
    unsigned char cmd[16]={0};
    cmd[0] = 0x02;  //sub command id of MMP16 (VIDEO OPTION)
    cmd[1] = 0x01;  //get command
    cmd[2] = 0x00;
    cmd[3] = 0x00;
    cmd[4] = 0x00;
    cmd[5] = 0x00;

    hr = AitXU_XuCmd(handle,cmd,EXU_CS_SET_MMP_CMD16,16,AIT_XU_CMD_OUT);
    if(hr<0)
        return hr;

    hr = AitXU_XuCmd(handle,cmd,EXU_CS_GET_MMP_RESULT16,16,AIT_XU_CMD_IN); //Get current status
    if(hr<0)
        return hr;

    *options = cmd[1] + cmd[2]*0x01 + cmd[3]*0x1000 + cmd[4]*0x1000000;

    return SUCCESS;
}

int AitXU_SetVideoOption(AitXUHandle handle, unsigned int options)
{
    int hr=0;
    unsigned char cmd[16] = {0};
    cmd[0] = 0x02;  //sub command id of MMP16 (VIDEO OPTION)
    cmd[1] = 0x02;  //set command
    cmd[2] = options & 0xFF;
    cmd[3] = (options>>8) & 0xFF;
    cmd[4] = (options>>16) & 0xFF;
    cmd[5] = (options>>24) & 0xFF;

    hr = AitXU_XuCmd(handle,cmd,EXU_CS_SET_MMP_CMD16,16,AIT_XU_CMD_OUT);
    if(hr<0)
        return hr;

    return SUCCESS;
}

int AitXU_EnableFD(AitXUHandle handle,unsigned char enable)
{
    unsigned int option;
    int hr = AitXU_GetVideoOption(handle,&option);
    if(hr<0)
        return hr;
    return AitXU_SetVideoOption(handle,option | VID_ENABLE_FD);
}

#define AF_POS_LIST_SIZE    81
#define AF_POS_LIST_MEM_BUFF_SIZE   (((81+(EXU1_CS_GET_DATA_LEN-1))/EXU1_CS_GET_DATA_LEN)*EXU1_CS_GET_DATA_LEN)
int AitXU_GetFD_Position(AitXUHandle handle,FacePositionList *face_pos_list)
{
    int hr = 0;
    if(handle == 0 )
        return -1;

    unsigned char cmd[EXU1_CS_GET_DATA_LEN];  //uvc xu command
    unsigned char *buf = new unsigned char[AF_POS_LIST_MEM_BUFF_SIZE]; //currentlly we fixed the face position data at 81 bytes

    //set ready to get fd pos command
    memset(cmd,0,EXU1_CS_GET_DATA_LEN);
    cmd[0] = EXU_MMP_RW_DATA_PARAM;  //mmp sub command
    cmd[1] = MMP_GET_FD_POS;  //set i2c address
    hr = AitXU_XuCmd(handle,cmd,EXU1_CS_SET_MMP,8,AIT_XU_CMD_OUT);

    //Get FD pos data
    memset(buf,0,AF_POS_LIST_MEM_BUFF_SIZE);
    for(int n=0;n<(AF_POS_LIST_MEM_BUFF_SIZE/EXU1_CS_GET_DATA_LEN);++n)
    {
        hr = AitXU_XuCmd(   handle,
                            buf+(n*EXU1_CS_GET_DATA_LEN),    //BUFFER
                            EXU1_CS_GET_DATA,
                            EXU1_CS_GET_DATA_LEN,
                            AIT_XU_CMD_IN);
    }

    unsigned char *ptr = buf;
    face_pos_list->num_face = *ptr++;

    for(int n=0;n<face_pos_list->num_face;++n)
    {
        face_pos_list->face_rect[n].left    = ptr[0] + ptr[1]*0x100;
        face_pos_list->face_rect[n].top     = ptr[2] + ptr[3]*0x100;
        face_pos_list->face_rect[n].right   = ptr[4] + ptr[5]*0x100;
        face_pos_list->face_rect[n].bottom  = ptr[6] + ptr[7]*0x100;
        ptr += 8;
    }
    delete buf;

    if(hr<0)
    {
        DbgMsg("Failed to get fd pos array: 0x%X",hr);
        return FAILED;
    }

    return SUCCESS;
}

int AitXU_TriggerMJPG(AitXUHandle handle)
{
    int hr=0;
    unsigned char cmd[16] = {0};
    cmd[0] = EXU1_MMP16_TRIGGER_MJPG;   //sub command id of MMP16 (VIDEO OPTION)
    cmd[1] = 0x00;
    hr = AitXU_XuCmd(handle,cmd,EXU_CS_SET_MMP_CMD16,16,AIT_XU_CMD_OUT);
    if(hr<0)
        return hr;
    return 0;
}


int AitUVC_CameraCmd(AitXUHandle handle,__s64 *val,__u32 id,unsigned char direction)
{
    DbgMsg("AitUVC_CameraCmd \r\n");

    v4l2_ext_controls controls;
    v4l2_ext_control control;

    AitXU *aitxu = (AitXU*)handle;

    int err=0;
    //bool need_close_dev = false;
    controls.ctrl_class = V4L2_CTRL_CLASS_USER;
    controls.count = 1;
    controls.error_idx = 0;
    controls.reserved[0] = 0;
    controls.reserved[1] = 0;
    controls.controls = &control;

    control.id  = id;
    control.reserved2[0] = 0;
    control.reserved2[1] = 0;
    control.value64 = *val;

    if( (err=ioctl(aitxu->dev, (direction==AIT_XU_CMD_IN)?VIDIOC_G_EXT_CTRLS:VIDIOC_S_EXT_CTRLS, &controls))<0 )
    {
        DbgMsg("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
        return errno;
    }

    if(direction==AIT_XU_CMD_IN)
    {
        *val = control.value64;
    }

    return 0;
}

int AitUVC_PUQuery(AitXUHandle handle,__u32 id,__s32 *max,__s32 *min,__s32 *step)
{
    struct v4l2_queryctrl queryctrl;
    AitXU *aitxu = (AitXU*)handle;

    memset (&queryctrl, 0, sizeof (queryctrl));
    queryctrl.id = id;

    if (-1 == ioctl (aitxu->dev, VIDIOC_QUERYCTRL, &queryctrl)) {
        if (errno != EINVAL) {
            perror ("VIDIOC_QUERYCTRL");
            return errno;
        } else {
            DbgMsg ("Control ID not supported\n");
            return -1;
        }
    } else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
        DbgMsg ("Control ID not supported\n");
        return -1;
    }

    *max = queryctrl.maximum;
    *min = queryctrl.minimum;
    *step = queryctrl.step;
    return 0;
}

int AitUVC_PUControl(AitXUHandle handle,__s32 *val,__u32 id,unsigned char direction) //processing unit control
{


    struct v4l2_control control;
    AitXU *aitxu = (AitXU*)handle;

    memset (&control, 0, sizeof (control));
    if(direction==AIT_XU_CMD_IN) //get value
    {
        control.id = id;

        if (-1 == ioctl (aitxu->dev, VIDIOC_G_CTRL, &control)) {
            perror ("VIDIOC_G_CTRL");
            return errno;
        }
        *val = control.value;

    } else {    //Set value
        control.id = id;
        control.value = *val;

        if (-1 == ioctl (aitxu->dev, VIDIOC_S_CTRL, &control)) {
            perror ("VIDIOC_S_CTRL");
            return errno;
        }
    }

    return 0;
}


int AitXU_UpdateFW(AitXUHandle handle,unsigned char* fw_data,unsigned int len)
{
	int hr;

	unsigned char DeviceRet[8]={0,0,0,0,0,0,0,0};
	unsigned char Cmd[8]={0,0,0,0,0,0,0,0};


	Cmd[0] = EXUID_FW_BURNING;//1;	//fw burning
	Cmd[1] = 0; //fw burning initialize

    hr = AitXU_XuCmd(handle,Cmd,EXU1_CS_SET_MMP,8,AIT_XU_CMD_OUT);

	if(hr!=0)
		return AITAPI_DEV_NOT_EXIST;

    hr = AitXU_XuCmd(handle,Cmd,EXU1_CS_GET_MMP_RESULT,8,AIT_XU_CMD_IN);
	if(DeviceRet[0]!=0)
		return AITAPI_NOT_SUPPORT;

	//send fw data
    AitXU_WriteFWData(handle,fw_data,len);

	Cmd[0] = 1;	//fw burning
	Cmd[1] = 1;	//fw burning finish
	//send mmp command

    hr = AitXU_XuCmd(handle,Cmd,EXU1_CS_SET_MMP,8,AIT_XU_CMD_OUT);
	if(hr!=0)
		return AITAPI_DEV_NOT_EXIST;

	int c=0;
	do {

            MsSleep(30);
            hr = AitXU_XuCmd(handle,Cmd,EXU1_CS_GET_MMP_RESULT,8,AIT_XU_CMD_IN);

            if(DeviceRet[0] == 0x82)//firmware burning error
            {
                DbgMsg("Firmware burning error\r\n");
                return AITAPI_NOT_SUPPORT;
            }
            else if(DeviceRet[0] == 0x00 && c==0)
            {
                for (int i = 0; i < 80; i++)
                {
                    //usleep(100*1000);
                    MsSleep(100);
                }
            }
		++c;

    } while (DeviceRet[0] != 0); //no error
     return SUCCESS;
}

void FW843x_Hotfix(AitXUHandle handle)
{
	unsigned char Value=0;
	AitXU_ReadReg(handle,0x6900,&Value);
	if( (Value&0x50)==0x50  ||  (Value&0x40)==0x40  )
	{
		AitXU_WriteReg(handle,0x6902,0x57); // SRAM 224KB

		AitXU_ReadReg(handle,0x6926,&Value);
		AitXU_WriteReg(handle,0x6926,Value|0x04); // SPI PAD enable

		AitXU_WriteReg(handle,0x6708,0x01); // clear SPI status
		AitXU_WriteReg(handle,0x6714,0x05); // SPI read status cmd
		AitXU_WriteReg(handle,0x6710,0xA8); // SPI start
		usleep(1000);

		AitXU_ReadReg(handle,0x6730,&Value);

		if(Value&0xFC) // write protect status
        {
			AitXU_WriteReg(handle,0x6708,0x01); // clear SPI status
			AitXU_WriteReg(handle,0x6714,0x06); // SPI write enable cmd
			AitXU_WriteReg(handle,0x6710,0x80); // SPI start
			usleep(1000);

			AitXU_WriteReg(handle,0x6708,0x01); // clear SPI status
			AitXU_WriteReg(handle,0x6714,0x01); // SPI write status cmd
			AitXU_WriteReg(handle,0x6720,0x00); // SPI write data
			AitXU_WriteReg(handle,0x6710,0x88); // SPI start
			usleep(1000);
		}

		AitXU_WriteReg(handle,0x6708,0x01); // clear SPI status
	}
}

int AitXU_UpdateFW_843X(AitXUHandle handle,unsigned char* fw_data,unsigned int len)
{
	int hr;

	unsigned char DeviceRet[8]={0,0,0,0,0,0,0,0};
	unsigned char Cmd[8]={0,0,0,0,0,0,0,0};


    FW843x_Hotfix(handle);

	Cmd[0] = EXUID_FW_BURNING;//1;	//fw burning
	Cmd[1] = 0; //fw burning initialize
	Cmd[2] = 3;

    hr = AitXU_XuCmd(handle,Cmd,EXU1_CS_SET_MMP,8,AIT_XU_CMD_OUT);

	if(hr!=0)
		return AITAPI_DEV_NOT_EXIST;

    hr = AitXU_XuCmd(handle,Cmd,EXU1_CS_GET_MMP_RESULT,8,AIT_XU_CMD_IN);
	if(DeviceRet[0]!=0)
		return AITAPI_NOT_SUPPORT;

	//send fw data
    AitXU_WriteFWData(handle,fw_data,len);

	Cmd[0] = 1;	//fw burning
	Cmd[1] = 1; //fw burning finish
	Cmd[2] = 3;
	//send mmp command

    hr = AitXU_XuCmd(handle,Cmd,EXU1_CS_SET_MMP,8,AIT_XU_CMD_OUT);
	if(hr!=0)
	{
	    //printf("Fw burn-in timeout.\r\n");
	    sleep(5);
		return SUCCESS;
	}

    sleep(6);


	return SUCCESS;
}


int AitXU_WriteFWData(AitXUHandle handle,unsigned char* data,unsigned int Len)
{
#define MAX_DATA_LEN 32

	int hr;
	int RemLen=Len;
	unsigned char *dataPtr = data;
	unsigned int ret = SUCCESS;

	unsigned char *buf = new unsigned char[MAX_DATA_LEN];

	if (SUCCESS == ret) {
		int count = 0;
		//printf("\r\n .");
	    while(RemLen>0)
	    {
		unsigned short len;
		memset(buf,0,MAX_DATA_LEN);

		if(RemLen>MAX_DATA_LEN)
		    len = MAX_DATA_LEN;
		else
		    len = RemLen;

		memcpy(buf,dataPtr,len);

		//data
		hr = AitXU_XuCmd(   handle,buf,
				    EXU1_CS_SET_FW_DATA,
				    MAX_DATA_LEN,
				    AIT_XU_CMD_OUT);
		if(hr!=0)
		{
		    delete buf;
		    return FAILED;
		}
		dataPtr += len;
		RemLen -= len;

		if(++count == 100)
		{
		    count = 0;
		}
	    }

	}

	delete buf;

	return ret;
#undef MAX_DATA_LEN
}

int AitXU_GetFWBuildDate(AitXUHandle handle,char *fwBuildDay)
{
	int hr;
	//unsigned int ByteRet=0;AitXU_XuCmdAitXU_XuCmd
	unsigned char DeviceRet[8]={0,0,0,0,0,0,0,0};
	unsigned char Cmd[8]={0,0,0,0,0,0,0,0};

	//AitDeviceConfig *dev_cfg = (AitDeviceConfig*) DevHandle;
	//IBaseFilter *dev = dev_cfg->GetSource();//(IBaseFilter*) DevHandle;

	Cmd[0] = EXUID_GET_FW_BUILDDAY;//12;
	/*
	hr = UvcExtSet(//	4,//node
					&PROPSETID_VIDCAP_EXTENSION_UNIT,//node guid
					EXU1_CS_SET_ISP,//req id
					Cmd,//command buffer
					8,//buffer len
					&ByteRet,
					dev);
	*/
    hr = AitXU_XuCmd(   handle,
                        Cmd,
                        EXU1_CS_SET_ISP,
                        8,
                        AIT_XU_CMD_OUT);

	if(hr!=0)
		return AITAPI_DEV_NOT_EXIST;

    /*
	hr=UvcExtGet(//	4,//node
				&PROPSETID_VIDCAP_EXTENSION_UNIT,//node guid
				EXU1_CS_GET_ISP_RESULT,//command id
				(BYTE*)&DeviceRet,//command buffer
				8,//buffer len
				&ByteRet,
				dev);
	*/

    hr = AitXU_XuCmd(   handle,
                        DeviceRet,
                        EXU1_CS_GET_ISP_RESULT,
                        8,
                        AIT_XU_CMD_IN);
	if(DeviceRet[0]!=0)
		return AITAPI_NOT_SUPPORT;

	fwBuildDay[0] =  '2';
	fwBuildDay[1] =  '0';
	fwBuildDay[2] =  DeviceRet[1];
	fwBuildDay[3] =  DeviceRet[2];
	fwBuildDay[4] =  '.';
	fwBuildDay[5] =  DeviceRet[3];
	fwBuildDay[6] =  DeviceRet[4];
	fwBuildDay[7] =  DeviceRet[5];
	fwBuildDay[8] =  '.';
	fwBuildDay[9] =  DeviceRet[6];
	fwBuildDay[10] =  DeviceRet[7];
	fwBuildDay[11] = 0;

	return SUCCESS;
}

int AitXU_GetFWVersion(AitXUHandle handle,unsigned char *fwVer)
{
	int hr;
	//unsigned int ByteRet=0;
	unsigned char DeviceRet[8]={0,0,0,0,0,0,0,0};
	unsigned char Cmd[8]={0,0,0,0,0,0,0,0};

	Cmd[0] = EXUID_GET_FW_VER;//11;	//change quality
	/*
	hr = UvcExtSet(//	4,//node
					&PROPSETID_VIDCAP_EXTENSION_UNIT,//node guid
					EXU1_CS_SET_ISP,//req id
					Cmd,//command buffer
					8,//buffer len
					&ByteRet,
					dev_cfg->GetSource());
    */
    hr = AitXU_XuCmd(   handle,
                        Cmd,
                        EXU1_CS_SET_ISP,
                        8,
                        AIT_XU_CMD_OUT);
	if(hr<0)
		return AITAPI_DEV_NOT_EXIST;

    /*
	hr=UvcExtGet(//	4,//node
				&PROPSETID_VIDCAP_EXTENSION_UNIT,//node guid
				EXU1_CS_GET_ISP_RESULT,//command id
				(BYTE*)&DeviceRet,//command buffer
				8,//buffer len
				&ByteRet,
				dev_cfg->GetSource());
	*/
    hr = AitXU_XuCmd(   handle,
                        DeviceRet,
                        EXU1_CS_GET_ISP_RESULT,
                        8,
                        AIT_XU_CMD_IN);
	if(DeviceRet[0]!=0)
		return AITAPI_NOT_SUPPORT;

	memcpy(fwVer,DeviceRet+2,6);
	return SUCCESS;
}

int AitXU_UpdateAudioData(AitXUHandle handle, unsigned char *audio_data, unsigned long len)
{
	int hr=0;
	unsigned char cmd[8] = {0};
	cmd[0] = 0x01;  //mmp sub command
	cmd[1] = 0x00;  //set i2c address
	cmd[2] = 0x01;  //codec i2c address
	cmd[3] = 2;

	//set I2C addr
	hr = AitXU_XuCmd(handle,cmd,EXU1_CS_SET_MMP,8,AIT_XU_CMD_OUT);
	if(hr<0)
		return hr;


	//get return value
	hr = AitXU_XuCmd(handle,cmd,EXU1_CS_GET_MMP_RESULT,8,AIT_XU_CMD_IN);
	if(hr<0)
		return hr;

	DbgMsg("%d\n" ,cmd[0]);

	//send fw data
	AitXU_WriteFWData(handle,audio_data,len);//,ProgressCB,callbackData);


	cmd[0] = 1; //fw burning
	cmd[1] = 1; //fw burning finish
	cmd[2] = 0;
	cmd[3] = 2; //Write audio data.


	hr = AitXU_XuCmd(handle,cmd,EXU1_CS_SET_MMP,8,AIT_XU_CMD_OUT);
	if(hr<0)
		return AITAPI_DEV_NOT_EXIST;

	int c=0;
	do {
		//DoEvent();
		//usleep(30000);//Sleep(30);
		MsSleep(30);


		hr = AitXU_XuCmd(handle,cmd,EXU1_CS_GET_MMP_RESULT,8,AIT_XU_CMD_IN);
		if(hr<0)
			return AITAPI_DEV_NOT_EXIST;

		if(cmd[0] == 0x82/*firmware burning error*/) {
			//MessageBox(0, TEXT("Firmware burning error"), TEXT("Error"), MB_OK);
			return AITAPI_NOT_SUPPORT;
		}else if(cmd[0] == 0x00 && c==0){
			for (int i = 0; i < 10; i++) {
				//Sleep(100);
				//usleep(10000);
				//DoEvent();
				//Sleep(1);
				MsSleep(10);
			}
		}
		++c;

	} while (cmd[0] != 0/*no error*/);



    return SUCCESS;
}

int AitXU_AudioPlayOp(AitXUHandle handle, unsigned char operation, unsigned short volume)
{
	int hr;
	unsigned char Cmd[8]={0,0,0,0,0,0,0,0};

	Cmd[0] = 0x0D;
	Cmd[2] = 0;
	switch(operation){
		case 0: //init audio play
			Cmd[1] = 0x00;
			break;
		case 1: //start audio play
			Cmd[1] = 0x01;
			break;
		case 2: //stop audio play
			Cmd[1] = 0x02;
			break;
		case 3: //set audio volume
			Cmd[1] = 0x02;
			Cmd[2] = (unsigned char)(volume & 0xFF);
			break;
		//case 4: //get audio volume //TBD
		//	Cmd[1] = 0x02;
		//	break;
		default:
			return AITAPI_DEV_NOT_EXIST;
			break;
	}

	hr = AitXU_XuCmd(handle,Cmd,EXU1_CS_SET_MMP,8,AIT_XU_CMD_OUT);
	if(hr<0)
		return AITAPI_DEV_NOT_EXIST;

	hr = AitXU_XuCmd(handle,Cmd,EXU1_CS_GET_MMP_RESULT,8,AIT_XU_CMD_IN);
	if(hr<0)
		return AITAPI_DEV_NOT_EXIST;

	return SUCCESS;
}

int AitXU_IspExCmd(AitXUHandle handle,uint8_t *cmd_in,uint8_t *cmd_out)
{
    DbgMsg("AitXU_IspExCmd\r\n");

    //AitXU *aitxu = (AitXU*)handle;

    if(cmd_in)
    {
        if( AitXU_XuCmd(handle,cmd_in,EXU1_CS_SET_ISP_EX,16,AIT_XU_CMD_OUT)<0 )
        {
            DbgMsg("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }
    }

    if(cmd_out)
    {
        //get result from device
        if( AitXU_XuCmd(handle,cmd_out,EXU1_CS_GET_ISP_EX_RESULT,16,AIT_XU_CMD_IN)<0 )
        {
            DbgMsg("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
            return errno;
        }
    }

    return 0;
}


int AitXU_SetMode(AitXUHandle handle,AIT_STREAM_MODE mode)
{
	  DbgMsg("SkypeXU_SetMode\r\n");

	  //    SkypeXU *skypexu = (SkypeXU*)handle;
	  errno = 0;

	  unsigned char cmd[8]={0};
	  cmd[0] = EXU_MMP_SKYPE_MODE;
	  cmd[1] = mode;

	  return AitXU_XuCmd(handle,cmd,EXU1_CS_SET_MMP,8,AIT_XU_CMD_OUT);

}


int AitXU_GetFlashType(AitXUHandle handle,uint8_t *cmd_out)
{
    int err=0x80000000;
    unsigned char cmd[8] = {0};

    cmd[0] = EXU_MMP_GET_FLASH_ID;

    err = AitXU_XuCmd(handle, cmd, EXU1_CS_SET_MMP,8,AIT_XU_CMD_OUT);
    if(err<0)
    {
        //ERROR("failed force I frame.\r\n");
        //ERROR("uvcioc ctrl set error: errno=%d (retval=%d)", errno, err);
        return AIT_XU_ERROR;
    }

    if( AitXU_XuCmd(handle,cmd_out,EXU1_CS_GET_MMP_RESULT,8,AIT_XU_CMD_IN)<0 )
    {
        DbgMsg("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
        return errno;
    }

    return AIT_XU_OK;
}



int AitXU_GetFlashSize(AitXUHandle handle,uint8_t *cmd_out)
{
    int err=0x80000000;
    unsigned char cmd[8] = {0};

    cmd[0] = EXU_MMP_GET_FLASH_SIZE;

    err = AitXU_XuCmd(handle, cmd, EXU1_CS_SET_MMP,8, AIT_XU_CMD_OUT);
    if(err<0)
    {
        //ERROR("failed force I frame.\r\n");
        //ERROR("uvcioc ctrl set error: errno=%d (retval=%d)", errno, err);
        return AIT_XU_ERROR;
    }

    if( AitXU_XuCmd(handle,cmd_out,EXU1_CS_GET_MMP_RESULT,8,AIT_XU_CMD_IN)<0 )
    {
        DbgMsg("Command Failed: errno=%d, %s\r\n",errno,strerror(errno));
        return errno;
    }
    return AIT_XU_OK;
}

int AitXU_GetFlashErase(AitXUHandle handle,unsigned int addr)
{
    int err=0x80000000;
    unsigned char cmd[8] = {0};

    cmd[0] = EXU_MMP_ERASE_FLASH;
    cmd[1] = addr&0xFF;
    cmd[2] = (addr>>8)&0xFF;
    cmd[3] = (addr>>16)&0xFF;
    cmd[4] = (addr>>24)&0xFF;

    err = AitXU_XuCmd(handle, cmd, EXU1_CS_SET_MMP,8,1);
    if(err<0)
    {
        //ERROR("failed force I frame.\r\n");
        //ERROR("uvcioc ctrl set error: errno=%d (retval=%d)", errno, err);
        return AIT_XU_ERROR;
    }
    return AIT_XU_OK;
}

int AitXU_SetWDR(AitXUHandle handle,unsigned int val)
{
    int err=0x80000000;
    unsigned char cmd[8] = {0};

    cmd[0] = EXUID_SET_WDR;
    cmd[1] = !(val==0);

    err = AitXU_XuCmd(handle, cmd, EXU1_CS_SET_ISP, 8, 1);
    if(err<0)
    {
        //ERROR("failed force I frame.\r\n");
        //ERROR("uvcioc ctrl set error: errno=%d (retval=%d)", errno, err);
        return AIT_XU_ERROR;
    }
    return AIT_XU_OK;
}
