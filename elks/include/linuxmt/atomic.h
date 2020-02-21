// Atomic primitives
// Architecture dependent

#pragma once

#include <linuxmt/types.h>

typedef volatile word_t atomic_t;

//
// Reference counting
//

typedef atomic_t count_t;

void count_up   (count_t * count);
void count_down (count_t * count);

//
// Locking (non reentrant mutexing)
//

typedef atomic_t lock_t;

// Returns 0 if lock acquired

volatile lock_t try_lock (lock_t * lock);

// Unconditional unlock

void unlock (lock_t * lock);
