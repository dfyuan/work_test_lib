#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>


#include "motor.h"
#include "visca.h"


#define IPRINTF(fmt, argv...)\
{\
    char buf[128] = {0};\
    snprintf(buf, sizeof(buf), "%s", __FILE__);\
    char *p = strrchr(buf, '/');\
	printf("INFO [%s %d] "fmt, p, __LINE__, ##argv);\
}

int main(void)
{	
	Visca_Init();

	while(1)
	{
		usleep(10*1000);
	}
	Visca_Uninit();

	return 0;
}

