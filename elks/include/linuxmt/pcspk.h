#ifndef __LINUXMT_PCSPK_H
#define __LINUXMT_PCSPK_H

#include <linuxmt/types.h>

#ifdef __KERNEL__
int pcspk_seq_ioctl(char *arg);
void pcspk_seq_stop(void);
void pcspk_seq_exit(pid_t pid);
#endif

#endif
