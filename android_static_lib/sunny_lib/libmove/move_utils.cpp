#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <utils/Log.h>
#include "log_tag.h"
#include "tty_utils.h"

int tty_init(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity)
{
	int err;

	if (tty_set(fd, 115200, 0, 8, 1, 'N') == FALSE) {
		return FALSE;
	} else {
		return TRUE;
	}
}

void V_Add(unsigned short *v)
{
	*v += 16;
	if (*v > 400) *v = 400;
	ALOGD("go fast, speed---> %0x\n", *v);
}

void V_Sub(unsigned short *v)
{
	*v -= 16;
	if (*v < 20) *v = 20;
	ALOGD("go slow, speed---> %0x\n", *v);
}

void R_Add(unsigned short *r)
{
	*r += 16;
	if (*r > 100) *r = 100;
	ALOGD("inc radius, radius---> %0x\n", *r);
}

void R_Sub(unsigned short *r)
{
	*r -= 16;
	if (*r < 1) *r = 1;
	ALOGD("dec radius, radius---> %0x\n", *r);
}

void go_forward(int fd, unsigned short v, unsigned char *data_buffer)
{
	int i;
	ALOGD("v = %x\n", v);

	data_buffer[5] = v & 0xff;
	data_buffer[6] = (v >> 8) & 0xff;
	data_buffer[7] = 0;
	data_buffer[8] = 0;

	for (i = 0 ; i < 10; i++) {
		ALOGD("%x ", data_buffer[i]);
	}
	ALOGD("\n");
	tty_send(fd, data_buffer, Data_Buffer_Size);
	ALOGD("go forward, speed---> %0x\n", v);
}

void go_backward(int fd, unsigned short v, unsigned char *data_buffer)
{
	int i;
	ALOGD("v = %x\n", v);
	v = 65535 - v;
	ALOGD("v = %x\n", v);
	data_buffer[5] = v & 0xff;
	data_buffer[6] = (v >> 8) & 0xff;
	data_buffer[7] = 0;
	data_buffer[8] = 0;

	for (i = 0 ; i < 10; i++) {
		ALOGD("%02x ", data_buffer[i]);
	}
	ALOGD("\n");
	tty_send(fd, data_buffer, Data_Buffer_Size);
	ALOGD("go backwark, speed：---> %0x\n", v);
}

void turn_left(int fd, unsigned short r, unsigned char *data_buffer)
{
	int i;
	ALOGD("r = %0x", r);
	data_buffer[5] = 0X60;
	data_buffer[6] = 0X00;
	data_buffer[7] = r & 0X00FF;
	data_buffer[8] = (r >> 8) & 0XFF;
	for (i = 0 ; i < 10; i++) {
		ALOGD("%02x ", data_buffer[i]);
	}
	ALOGD("\n");
	tty_send(fd, data_buffer, Data_Buffer_Size);
}

void turn_right(int fd, unsigned short r, unsigned char *data_buffer)
{
	int i;
	r = 65535 - r;
	ALOGD("%0x\n", r);
	data_buffer[5] = 0X60;
	data_buffer[6] = 0X00;
	data_buffer[7] = r & 0X00FF;
	data_buffer[8] = (r >> 8) & 0XFF;
	ALOGD("%0x\n", r >> 8);
	for (i = 0 ; i < 10; i++) {
		ALOGD("%02x ", data_buffer[i]);
	}
	ALOGD("\n");
	tty_send(fd, data_buffer, Data_Buffer_Size);
}

void *move_test_thread(void *arg)
{
	int fd;
	int err;
	int len, i;
	char rcv_buf[100];
	unsigned char send_buf[] = {0XAA, 0X55, 0X06, 0X01, 0X04, 0X40, 0X00, 0X40, 0X00, 0X00};
	unsigned short R, V;

	char *dev  = "/dev/ttyS1";
	V = 0X0040;
	R = 0X0040;

	fd = tty_open(dev);
	do {
		err = tty_init(fd, 115200, 0, 8, 1, 'N');
	} while (FALSE == err || FALSE == fd);

	while (1) {
		go_forward(fd, V, send_buf);
		sleep(1);
		go_backward(fd, V, send_buf);
		sleep(1);
		turn_left(fd, R, send_buf);
		sleep(1);
		turn_right(fd, R, send_buf);
		sleep(1);
		V_Sub(&V);
		sleep(1);
		V_Add(&V);
		sleep(1);
		R_Sub(&R);
		sleep(1);
		ALOGD("R = %0x\n", R);
		R_Add(&R);
		sleep(1);
		ALOGD("R = %0x\n", R);
	}

	close(fd);
}
