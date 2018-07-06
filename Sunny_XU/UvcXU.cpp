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

#include <linux/version.h>

#include "UvcXU.hpp"
#include "AitXuDef.hpp"
#include "video.h"
#undef DEBUG_MSG

static __u32 uvc_k_version = KERNEL_VERSION(2,6,32);

void UVC_SetUVCKernelVersion(__u32 version)
{
  if((version<=KERNEL_VERSION(2,6,32)))
    uvc_k_version = KERNEL_VERSION(2,6,32);
  else if((version>KERNEL_VERSION(3,3,0)))
    uvc_k_version = KERNEL_VERSION(3,2,0);
  else
    uvc_k_version = version;
}

__u32 UVC_GetUVCKernelVersion(void)
{
  return uvc_k_version ;
}


int MsSleep(unsigned int t)
{
    struct timeval delay;
    delay.tv_sec = 0;
    delay.tv_usec = t * 1000; // 20 ms
    return select(0, NULL, NULL, NULL, &delay);
}
//static

//static
int xioctl(int fh, int request, void *arg) {
  int r;

  //Try command over if interrupted or it needs to run again
  do {
    r = ioctl(fh, request, arg);
  } while (-1 == r && (EINTR == errno || EAGAIN == errno) );

  return r;

}


static int UVC_XuCmd(int handle,unsigned char* cmd,unsigned short cs,unsigned char len, unsigned long direction, unsigned char unit)
{

    #ifdef DEBUG_MSG
    DbgMsg("UVC_XuCmd \r\n");
    #endif

    if(cmd == 0)
	  return EINVAL;

    int err=0;
    uvc_xu_control xu_ctrl;
    xu_ctrl.unit = unit;
    xu_ctrl.selector = cs;
    xu_ctrl.size = len;
    xu_ctrl.data = cmd;//buffer;
    //errno = 0;
    if( (err=ioctl(handle,direction,&xu_ctrl))<0 )
    {
    #ifdef DEBUG_MSG
//        DbgMsg("Command Failed: errno=%d, %s, ioctl err=0x%X \r\n",errno,strerror(errno),err);
    #endif
        return err;
    }
    #ifdef DEBUG_MSG
    DbgMsg("UVC_XuCmd err=0x%X \r\n",err);
    #endif
    return 0;
}

//XU query wrapper functions
s32 run_xu_query_3_0( s32 fd, struct uvc_xu_control_query_3_2_0 *xu_q ) {
  s32 ret = 0;

  ret = xioctl( fd, UVCIOC_CTRL_QUERY_3_2_0, xu_q);
  return ret;
}

s32 run_xu_query_3_2( s32 fd, struct uvc_xu_control_query_3_2_0 *xu_q ) {
  s32 ret = 0;
  ret = xioctl( fd, UVCIOC_CTRL_QUERY_3_2_0, xu_q);
  return ret;
}


int UVC_XuCmd_V2(int handle,unsigned char* cmd,unsigned short cs,unsigned char len, unsigned int query, unsigned char unit)
{
// 3.2 UVCIOC_CTRL_QUERY = 0xc00c7521

    struct uvc_xu_control_query_3_2_0 xu_ctrl_query;
		
    #ifdef DEBUG_MSG
    printf("UVC_XuCmd V2\r\n");
    #endif

    if(cmd == 0)
        return EINVAL;

    int err=0;


//    struct uvc_xu_control_query xu_ctrl_query;
    xu_ctrl_query.query = query;
    xu_ctrl_query.selector = cs;
    xu_ctrl_query.size = len;
    xu_ctrl_query.unit = unit;
    xu_ctrl_query.data = cmd;

//printf("size: xqry->query[%d] .\n",sizeof(xu_ctrl_query1.query));
//printf("size: xqry->unit[%d] .\n",sizeof(xu_ctrl_query1.unit));	  
//printf("size: xqry->selector[%d] .\n",sizeof(xu_ctrl_query1.selector));	  
//printf("size: xqry->size[%d] .\n",sizeof(xu_ctrl_query1.size));	  
//printf( "size: xqry->data[%d] .\n",sizeof(xu_ctrl_query1.data));	 

    
printf("size: xqry->query[%d] .\n",sizeof(xu_ctrl_query.query));
printf("size: xqry->unit[%d] .\n",sizeof(xu_ctrl_query.unit));	  
printf("size: xqry->selector[%d] .\n",sizeof(xu_ctrl_query.selector));	  
printf("size: xqry->size[%d] .\n",sizeof(xu_ctrl_query.size));	  
printf( "size: xqry->data[%d] .\n",sizeof(xu_ctrl_query.data));	    

printf("addr unit[%x] .\n",&xu_ctrl_query.unit);  	  
printf("addr selector[%x] .\n",&xu_ctrl_query.selector);
printf("addr query[%x] .\n",&xu_ctrl_query.query);
printf("addr size[%x] .\n",&xu_ctrl_query.size);	  
printf( "addr data[%x] .\n",&xu_ctrl_query.data);	


    if(uvc_k_version>=KERNEL_VERSION(3,2,0))
    {	 
   		 printf("3.2\n");
        err=run_xu_query_3_2((s32)handle, &xu_ctrl_query);
    }
    else if(uvc_k_version>=KERNEL_VERSION(3,0,0))
    {
		printf("3.0\n");

	 err=run_xu_query_3_0(handle, &xu_ctrl_query);
    }else
    {
      	printf("2.6\n");

    
          int err=0;

	  struct uvc_xu_control uvc_xu_ctrl_2_6;
	  uvc_xu_ctrl_2_6.unit = unit;
	  uvc_xu_ctrl_2_6.selector = cs;
	  uvc_xu_ctrl_2_6.size = len;
	  uvc_xu_ctrl_2_6.data = cmd;
	
      err=ioctl(handle,(query==UVC_GET_CUR)?UVCIOC_CTRL_GET:UVCIOC_CTRL_SET,&uvc_xu_ctrl_2_6);
      
    }

    if(err<0)
    {
	      printf("Command Failed: ret = %d\n",err);// errno=%d, %s\r\n",errno,errno(errno));
	      return errno;
    }
//    printf("XU Cmd ret = %d\n",err);
    return 0;
}
