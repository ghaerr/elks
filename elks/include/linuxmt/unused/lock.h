// Locking primitives

#ifndef __LINUXMT_LOCK_H
#define __LINUXMT_LOCK_H

#include <linuxmt/atomic.h>

// Wait for lock (not interruptible)

void wait_lock (lock_t * lock);
#define lock_wait wait_lock

// Unlock with event

void event_unlock (lock_t * lock);
#define unlock_event event_unlock

#endif
