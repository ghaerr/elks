#ifndef DEVETH_H
#define DEVETH_H

void deveth_printhex(char* packet, int len);
int  deveth_init(char *fdev);
void deveth_process(void);
void deveth_send(char *packet, int len);

#endif