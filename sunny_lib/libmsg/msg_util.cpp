#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "msg_util.h"
#define UNIX_DOMAIN "/data/UNIX.domain"

static int connect_fd;
int msg_init(void)
{
	struct sockaddr_un srv_addr;
	int ret = 0;
	//creat unix socket
	connect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (connect_fd < 0) {
		perror("cannot create communication socket");
		return -1;
	}
	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, UNIX_DOMAIN);
	//connect server
	socklen_t socklen = sizeof(srv_addr);
	ret = connect(connect_fd, (struct sockaddr*)&srv_addr, socklen);
	if (ret == -1) {
		perror("cannot connect to the server");
		close(connect_fd);
		return -1;
	}

	return 0;
}
int msg_send(void)
{
	struct tagmsg_buff msg_buff;
	msg_buff.cmd=0x55;//tetst
	msg_buff.data=0x01;
	write(connect_fd, &msg_buff, sizeof(struct tagmsg_buff));
	return 0;
}
int msg_uninit(void)
{
	close(connect_fd);
	return 0;
}