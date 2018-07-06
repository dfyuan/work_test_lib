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

static int speed_arr[] = {
							B50, B75, B110, B134, B150, B200, B300, B600, B1200, B1800, B2400,
							B4800, B9600, B19200, B38400, B57600, B115200, B230400
                         };
static int name_arr[] = {
							50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400,
							4800, 9600, 19200, 38400, 57600, 115200, 230400
                        };

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

int set_speed(int fd, int speed)
{
	int   i;
	int   ret = -1;
	struct termios options;

	tcgetattr(fd, &options);
	for (i = 0; i < sizeof(speed_arr)/sizeof(int); i++) {
		if (speed == name_arr[i]) {
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&options, speed_arr[i]);
			cfsetospeed(&options, speed_arr[i]);

			if (tcsetattr(fd, TCSANOW, &options) != 0) {
				printf("tcsetattr err\n");
				break;
			}
			ret = 0;
			break;
		}
		tcflush(fd, TCIOFLUSH);
	}

	return ret;
}

int set_parity(int fd, int databits, int stopbits, int parity)
{
	struct termios options;

	if (tcgetattr(fd, &options)  !=  0) {
		printf("tcgetattr err\n");
		return -1;
	}
	/* init or clear don't know now */
	options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
	                     | INLCR | IGNCR | ICRNL | IXON);
	options.c_oflag &= ~OPOST;
	options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	options.c_cflag &= ~(CSIZE | PARENB);

	options.c_cflag &= ~CSIZE;
	switch (databits) {
	case 7:
		options.c_cflag |= CS7;
		break;
	case 8:
		options.c_cflag |= CS8;
		break;
	default:
		fprintf(stderr, "Unsupported data size\n");
		return -1;
	}
	switch (parity) {
	case 'n':
	case 'N':
		options.c_cflag &= ~PARENB;   /* Clear parity enable */
		options.c_iflag &= ~INPCK;     /* Enable parity checking */
		break;
	case 'o':
	case 'O':
		options.c_cflag |= (PARODD | PARENB);
		options.c_iflag |= INPCK;             /* Disnable parity checking */
		break;
	case 'e':
	case 'E':
		options.c_cflag |= PARENB;     /* Enable parity */
		options.c_cflag &= ~PARODD;
		options.c_iflag |= INPCK;       /* Disnable parity checking */
		break;
	case 'S':
	case 's':  /*as no parity*/
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		break;
	default:
		fprintf(stderr, "Unsupported parity\n");
		return -1;
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
		return -1;
	}
	/* Set input parity option */
	if (parity != 'n')
		options.c_iflag |= INPCK;
	options.c_cc[VTIME] = 10; // 1 seconds
	options.c_cc[VMIN] = 0;

	tcflush(fd, TCIFLUSH); /* Update the options and do it NOW */
	if (tcsetattr(fd, TCSANOW, &options) != 0) {
		perror("SetupSerial 3");
		return -1;
	}
	return 0;
}

int tty_set(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity)
{
	set_speed(fd, speed);
	set_parity(fd, databits, stopbits, parity);

	return 0;
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
