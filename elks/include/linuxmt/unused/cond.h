#ifndef __LINUXMT_COND_H
#define __LINUXMT_COND_H

// Condition object

// Implement a generic condition for the need of scheduling
// to allow composite ones through indirection (poll and select cases)
// while saving code size (see issue #222)

typedef __u16 bool_t;	//FIXME should not introduce boolean type here

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
