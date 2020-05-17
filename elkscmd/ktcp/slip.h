#ifndef __SLIP_H__
#define __SLIP_H__

#define SLIP_MTU   1064

void slip_process(void);
int slip_init(char *fdev, speed_t baudrate);
void slip_send(unsigned char *packet, int len);

#endif
