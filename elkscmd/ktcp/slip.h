#ifndef __SLIP_H__
#define __SLIP_H__

//#define SLIP_MTU   1064		/* default too large for 1024 serial ring buffer*/
#define SLIP_MTU   1024

void slip_process(void);
int slip_init(char *fdev, speed_t baudrate);
void slip_send(unsigned char *packet, int len);

#endif
