#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "visca.h"
#include "motor.h"


#define DEV_TTYS1 "/dev/ttyS1"


int uart1_fd;
unsigned char uart1_rxbuf[1024]={0};

pthread_t g_visca_thread; 

void print_recv_buf(unsigned char *buf, int bufSize)
{
    if(buf == NULL)
    {
        printf("buf is NUll\n");
        return;
    }

    int i = 0;
    printf("recv buf :");
    for(i=0; i<bufSize; i++)
    {
        printf("0x%02x ", buf[i]);
    }
    printf("\n");
}
static int set_Parity(int fd,int databits,int stopbits,int parity)
{ 
	struct termios options; 
	if ( tcgetattr( fd,&options) != 0) 
	{ 
		perror("SetupSerial 1"); 
		return(0); 
	}

	options.c_cflag &= ~CSIZE; 
	switch (databits)
	{ 
		case 7: 
			options.c_cflag |= CS7; 
		break;
		case 8: 
			options.c_cflag |= CS8;
		break; 
		default: 
			fprintf(stderr,"Unsupported data sizen"); 
		return (0); 
	}

	switch (parity) 
	{ 
		case 'n':
		case 'N': 
		options.c_cflag &= ~PARENB; 
		options.c_iflag &= ~INPCK;
		break; 
		case 'o': 
		case 'O': 
		options.c_cflag |= (PARODD | PARENB);
		options.c_iflag |= INPCK; 
		break; 
		case 'e': 
		case 'E': 
		options.c_cflag |= PARENB; 
		options.c_cflag &= ~PARODD; 
		options.c_iflag |= INPCK;
		break;
		case 'S': 
		case 's':  
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;break; 
		default: 
		fprintf(stderr,"Unsupported parityn"); 
		return (0); 
		} 
	
		switch (stopbits)
		{ 
			case 1: 
			options.c_cflag &= ~CSTOPB; 
			break; 
			case 2: 
			options.c_cflag |= CSTOPB; 
			break;
			default: 
			fprintf(stderr,"Unsupported stop bitsn"); 
			return 0; 
		} 

	if(parity != 'n') 
		options.c_iflag |= INPCK; 
	
	tcflush(fd,TCIFLUSH);
	options.c_cc[VTIME] = 0; 
	options.c_cc[VMIN] = 0; 
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); 
	options.c_oflag &= ~(OPOST);
	 
	options.c_cflag |= (CLOCAL | CREAD);  
	options.c_iflag &= ~ (INLCR | ICRNL | IGNCR);
	options.c_oflag &= ~(ONLCR | OCRNL);
	options.c_iflag &= ~(IXON);

	if(tcsetattr(fd,TCSANOW,&options) != 0) 
	{ 
		perror("SetupSerial 3"); 
		return 0; 
	} 
	return 1; 
}

static int set_speed(int fd, int speed)
{
	int i; 
	int status; 
	struct termios Opt;
	const int speed_arr[] = { B38400, B19200, B9600, B4800, B2400, B1200, B300,B115200};
	const int name_arr[] =  {38400, 19200, 9600, 4800, 2400, 1200, 300, 115200};

	tcgetattr(fd, &Opt); 
	for ( i= 0; i < sizeof(speed_arr) / sizeof(int); i++)
	{ 
		if (speed == name_arr[i])
		{ 
				tcflush(fd, TCIOFLUSH); 
				cfsetispeed(&Opt, speed_arr[i]); 
				cfsetospeed(&Opt, speed_arr[i]); 
				status = tcsetattr(fd, TCSANOW, &Opt); 
				if (status != 0) 
				{ 
					printf("tcsetattr failed\n"); 
					return -1; 
				} 
				tcflush(fd,TCIOFLUSH); 
				return 0;
		}

	}
	return -1;
}
static void init_uart1(void)
{
	
	uart1_fd = open("/dev/ttyS1",O_RDWR);
	if(uart1_fd <= -1)
	{
		printf("Open tty12 Failed!\n");
		return;
	}

	if(set_speed(uart1_fd ,9600))
	{
		printf("Uart1 set_speed Failed! \n");
		close(uart1_fd);
		uart1_fd = -1;
		return ;
	}
	
	if(set_Parity(uart1_fd ,8,1,'N') == 0)
	{
		printf("Uart1 Set Parity Failed!\n");
		close(uart1_fd);
		uart1_fd = -1;
	}
	return ;
}
static void visca_set_zoom_handle(unsigned char data)
{
	s16 zoom_pos;
	if(data==0x02)
	{
		printf("Zoom OUT  \n");
		zoom_pos=get_zoom_cur_pos();
		zoom_pos = zoom_pos + 10;
		
		if(zoom_pos > ZM_POS_MAX)
			zoom_pos=ZM_POS_MAX;
		
		az_motor_goto_pos(focus_pos[ZM_POS_MAX - zoom_pos] + focus_offset.af, AF_SPEED, zoom_pos, ZM_SPEED);

	}
	else if(data==0x03)
	{
		printf("Zoom IN\n");
		zoom_pos=get_zoom_cur_pos();
		zoom_pos = zoom_pos - 10;
		
		if(zoom_pos < ZM_POS_MIN)
			zoom_pos=ZM_POS_MAX;
		
		az_motor_goto_pos(focus_pos[ZM_POS_MAX - zoom_pos] + focus_offset.af, AF_SPEED, zoom_pos, ZM_SPEED);	
	}
}
static void visca_set_focus_handle(unsigned char data)
{
	s16 focus_pos,zoom_pos;

	zoom_pos=get_zoom_cur_pos();
	if(data==0x02)
	{
		printf("Fucos Near!\n");
		focus_pos=get_focus_cur_pos();
		focus_pos = focus_pos + 2;
		
		if(focus_pos > AF_POS_MAX)
			focus_pos=AF_POS_MAX;
		az_motor_goto_pos(focus_pos, AF_SPEED, zoom_pos, ZM_SPEED);

	}
	else if(data==0x03)
	{
		printf("Fucos Far! \n");
		focus_pos=get_focus_cur_pos();
		focus_pos = focus_pos - 2;
		
		if(focus_pos < AF_POS_MIN)
			focus_pos=AF_POS_MIN;
		
		az_motor_goto_pos(focus_pos, AF_SPEED, zoom_pos, ZM_SPEED);	
	}


}
static void visca_set_awb_handle(unsigned char data)
{


}
static void visca_set_iris_handle(unsigned char data)
{
	u16 iris_val;
	if(data==0x02)
	{
		iris_val=motor_get_irisval();
		iris_val+=10;
		motor_set_irisval(iris_val);
		iris_val=motor_get_irisval();
		printf("Iris UP current=%d \n",iris_val);
	}
	else if(data==0x03)
	{
		iris_val=motor_get_irisval();
		iris_val-=10;
		motor_set_irisval(iris_val);
		iris_val=motor_get_irisval();
		printf("Iris Down current=%d \n",iris_val);
	}
}

static void analyse_uart1_data(int len,unsigned char *buffer)
{
	int i;

	for(i=0;i < len - 5;i++)
	{
		if(buffer[0+i]=0x81&&(buffer[5+i]==0xff))
		{
			switch (buffer[i+3])
			{
				case 0x07:
					visca_set_zoom_handle(buffer[i+4]);
					break;
				case 0x08:
					visca_set_focus_handle(buffer[i+4]);
					break;
				case 0x35:
					visca_set_awb_handle(buffer[i+4]);
					break;
				case 0x0b:
					visca_set_iris_handle(buffer[i+4]);
					break;
				default:
					printf("Unkown VISCA CMD!\n");
					break;

			}
		}
	}
}

static void uart1_read(void)
{
	int    retval;
	int    nreadByte;
	fd_set rfds;
	struct timeval tv ;

	if(uart1_fd<=0)
		return;

	FD_ZERO(&rfds);
    FD_SET(uart1_fd,&rfds);

	do{  
		tv.tv_sec = 0;
    	tv.tv_usec = 150000;
		FD_ZERO(&rfds);
		FD_SET(uart1_fd,&rfds); 
		retval = select(uart1_fd+1,&rfds,NULL,NULL,&tv);
		if(retval <= 0) 
        {	
			if(retval==-1)
      		{
      			printf(" select uart0 error,%d !\n",retval);
      		    if(uart1_fd>0)
      			{
					close(uart1_fd);
					uart1_fd = -1;
      			}
				init_uart1();	
			}
            continue;
        }
		else
        {
	  		if(FD_ISSET(uart1_fd, &rfds))
        	{ 
				nreadByte=read(uart1_fd, uart1_rxbuf, 1024);
				analyse_uart1_data(nreadByte,uart1_rxbuf);
			}
		}
	}while(0);
}
static void *visca_thread(void* arg)
{	
	init_uart1();
	while(1)
	{
		uart1_read();
		usleep(150*1000);
	}
}

void Visca_Init(void)
{
	int retval;

	retval = pthread_create(&g_visca_thread, NULL, visca_thread, NULL);
	if(retval != 0)
	{
		printf("Create VISCA Thread Failed! Error=%d\n", retval);
	}
}
void Visca_Uninit(void)
{
	pthread_join(g_visca_thread, NULL);
}
