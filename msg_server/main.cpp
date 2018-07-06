#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#define UNIX_DOMAIN "/data/UNIX.domain"
struct tagmsg_buff{
	int cmd;
	int data;
};

int main(void)
{
	socklen_t clt_addr_len;
	int listen_fd;
	int com_fd;
	int ret;
	int i;
	//static char recv_buf[1024];
	struct tagmsg_buff recv_buf;
	unsigned int len;
	struct sockaddr_un clt_addr;
	struct sockaddr_un srv_addr;
	listen_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		perror("cannot create communication socket");
		return 1;
	}

	//set server addr_param
	srv_addr.sun_family = AF_UNIX;
	strncpy(srv_addr.sun_path, UNIX_DOMAIN, sizeof(srv_addr.sun_path) - 1);
	unlink(UNIX_DOMAIN);
	//bind sockfd & addr
	ret = bind(listen_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
	if (ret == -1) {
		perror("cannot bind server socket");
		close(listen_fd);
		unlink(UNIX_DOMAIN);
		return 1;
	}
	//listen sockfd
	ret = listen(listen_fd, 1);
	if (ret == -1) {
		perror("cannot listen the client connect request");
		close(listen_fd);
		unlink(UNIX_DOMAIN);
		return 1;
	}
	//have connect request use accept
	len = sizeof(clt_addr);
	com_fd = accept(listen_fd, (struct sockaddr*)&clt_addr, &len);
	if (com_fd < 0) {
		perror("cannot accept client connect request");
		close(listen_fd);
		unlink(UNIX_DOMAIN);
		return 1;
	}
	//read and printf sent client info
	printf("/n=====info=====/n");
	memset(&recv_buf,0x0,sizeof(struct tagmsg_buff));
	int num = read(com_fd, &recv_buf, sizeof(struct tagmsg_buff));
	if (recv_buf.cmd == 0x55 && recv_buf.data == 0x1)
	{
		sleep(1);
		system("pgrep slam_demo | xargs kill -s 9");
		sleep(1);
		system("rmmod uvc_gadget.ko");
		sleep(1);
		system("rmmod ezy_buf.ko");
		sleep(1);
		system("insmod /system/lib/modules/usb_f_mass_storage.ko");
		sleep(1);
		system("insmod /system/lib/modules/g_mass_storage.ko file=/dev/block/mmcblk1p13 removable=1");
		close(com_fd);
		close(listen_fd);
		unlink(UNIX_DOMAIN);
	}


	return 0;
}
