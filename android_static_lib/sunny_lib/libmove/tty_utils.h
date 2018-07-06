#ifndef TTY_UTILS_H_
#define TTY_UTILS_H_

#define FALSE  -1
#define TRUE   0
#define Data_Buffer_Size 10

extern int tty_open(char* port);
extern void tty_close(int fd);
extern int tty_set(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity);
extern int tty_recv(int fd, char *rcv_buf, int data_len);
extern int tty_send(int fd, unsigned char *send_buf, int data_len);

#endif