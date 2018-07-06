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
#include "tty_utils.h"

int tty_open(char* port)
{
	int fd;

	fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
	if (FALSE == fd) {
		perror("Can't open Serial Port");
		return (FALSE);
	}
	return fd;
}

void tty_close(int fd)
{
	close(fd);
}

int tty_set(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity)
{
	int i;
	int status;
	int speed_arr[] = {B115200, B19200, B9600, B4800, B2400, B1200, B300};
	int name_arr[] = {115200,  19200,  9600,  4800,  2400,  1200,  300};

	struct termios options;

	if (tcgetattr(fd, &options) != 0) {
		perror("setupSerial 1");
		return (FALSE);
	}

	for (i = 0; i < sizeof(speed_arr) / sizeof(int); i++) {
		if (speed == name_arr[i]) {
			cfsetispeed(&options, speed_arr[i]);
			cfsetospeed(&options, speed_arr[i]);
		}
	}

	options.c_cflag |= CLOCAL;
	options.c_cflag |= CREAD;

	switch (flow_ctrl) {
	case 0 :
		options.c_cflag &= ~CRTSCTS;
		break;
	case 1 :
		options.c_cflag |= CRTSCTS;
		break;
	case 2 :
		options.c_cflag |= IXON | IXOFF | IXANY;
		break;
	}

	options.c_cflag &= ~CSIZE;
	switch (databits) {
	case 5:
		options.c_cflag |= CS5;
		break;
	case 6:
		options.c_cflag |= CS6;
		break;
	case 7:
		options.c_cflag |= CS7;
		break;
	case 8:
		options.c_cflag |= CS8;
		break;
	default:
		fprintf(stderr, "Unsupported data size\n");
		return (FALSE);
	}

	switch (parity) {
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
	case 's':
	case 'S':
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		break;
	default:
		fprintf(stderr, "Unsupported parity\n");
		return (FALSE);
	}

	switch (stopbits) {
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		fprintf(stderr, "Unsupported stop bits\n");
		return (FALSE);
	}

	options.c_oflag &= ~OPOST;
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	//options.c_lflag &= ~(ISIG | ICANON);

	options.c_cc[VTIME] = 1;
	options.c_cc[VMIN] = 1;

	tcflush(fd, TCIFLUSH);

	if (tcsetattr(fd, TCSANOW, &options) != 0) {
		perror("com set error!\n");
		return (FALSE);
	}
	return (TRUE);
}

int tty_recv(int fd, char *rcv_buf, int data_len)
{
	int len, ret;
	fd_set fs_read;

	struct timeval time;

	FD_ZERO(&fs_read);
	FD_SET(fd, &fs_read);

	time.tv_sec = 10;
	time.tv_usec = 0;

	ret = select(fd + 1, &fs_read, NULL, NULL, &time);
	if (ret) {
		len = read(fd, rcv_buf, data_len);
		return len;
	} else {
		return FALSE;
	}
}

int tty_send(int fd, unsigned char *send_buf, int data_len)
{
	int len = 0;
	len = write(fd, send_buf, data_len);
	if (len == data_len) {
		return len;
	} else {
		tcflush(fd, TCOFLUSH);
		return FALSE;
	}
}