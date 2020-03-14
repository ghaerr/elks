// Wait functions

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

// Condition object

// Implement a generic condition for the need of scheduling
// to allow composite ones through indirection (poll and select cases)
// while saving code size (see issue #222)

typedef __u16 bool_t;

typedef bool_t (* test_t) (void *);

struct cond_s
{
	test_t test;
	void * obj;
};

typedef struct cond_s cond_t;

#define cond_test(_c) \
	(((_c)->test) ((_c)->obj))

// Wait for condition change (= event)
// ELKS has no wait queue (simplification)

bool_t _wait_event (cond_t * c, bool_t i);

#define wait_event(_c) _wait_event ((_c), 0)
#define wait_event_interruptible(_c) _wait_event ((_c), 1)

#endif
