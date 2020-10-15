#ifndef __LINUXMT_WAIT_H
#define __LINUXMT_WAIT_H

#include <linuxmt/types.h>

#define WNOHANG		0x00000001
#define WUNTRACED	0x00000002

#define wake_up(_a) _wake_up(_a,1)
#define wake_up_interruptible(_a) _wake_up(_a,0)

struct wait_queue {
    char pad;
};

/* The special queue for selecting / polling */
extern struct wait_queue select_queue;

#endif
