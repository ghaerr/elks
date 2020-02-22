// Locking primitives

#pragma once

#include <linuxmt/atomic.h>

// Wait for lock (not interruptible)

void wait_lock (lock_t * lock);

// Unlock with event

void event_unlock (lock_t * lock);
