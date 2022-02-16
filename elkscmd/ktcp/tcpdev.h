#ifndef TCPDEV_H
#define TCPDEV_H

extern int tcpdevfd;

void tcpdev_process(void);
int tcpdev_init(char *fdev);
void notify_sock(void *sock, int type, int value);
void notify_data_avail(struct tcpcb_s *cb);
void retval_to_sock(void *sock, int r);
void tcpdev_notify_accept(struct tcpcb_s *cb);

#endif
