#ifndef __MSG_UTIL_H__
#define __MSG_UTIL_H__
struct tagmsg_buff{
	int cmd;
	int data;
};
int msg_init(void);
int msg_send(void);
int msg_uninit(void);
#endif