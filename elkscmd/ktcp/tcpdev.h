#ifndef TCPDEV_H
#define TCPDEV_H

extern int tcpdevfd;

void tcpdev_process(void);
int tcpdev_init(char *fdev);
void retval_to_sock(void *sock, short int r);
void tcpdev_checkread(struct tcpcb_s *cb);
void tcpdev_sock_state(struct tcpcb_s *cb, int state);
void tcpdev_checkaccept(struct tcpcb_s *cb);

#endif
