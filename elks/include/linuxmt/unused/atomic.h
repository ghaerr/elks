// Atomic primitives
// Architecture dependent

#ifndef __LINUXMT_ATOMIC_H
#define __LINUXMT_ATOMIC_H

#include <linuxmt/types.h>

//
// Atomic operations
//

typedef volatile word_t atomic_t;

atomic_t atomic_get (atomic_t * count);

void atomic_inc (atomic_t * count);
void atomic_dec (atomic_t * count);

//
// Locking (non reentrant mutexing)
//

typedef atomic_t lock_t;

// Returns 0 if lock acquired

volatile lock_t try_lock (lock_t * lock);

// Unconditional unlock

void unlock (lock_t * lock);

#endif
