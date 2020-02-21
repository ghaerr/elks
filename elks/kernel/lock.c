
#include <linuxmt/sched.h>
#include <linuxmt/wait.h>
#include <linuxmt/lock.h>

// Lock condition test

static bool_t test_lock (void * lock)
{
	return !try_lock ((lock_t *) lock);  // 0: acquired
}

// Wait for lock

void wait_lock (lock_t * lock)
{
	// First try to save from wait_event overhead

	if (try_lock (lock))  // 1: not acquired
	{
		// The lock is acquired as soon as condition is verified

		cond_t cond = {.test = test_lock, .obj = (void *) lock};
		wait_event (&cond);  // uninterruptible
	}
}

// Unlock with event

void event_unlock (lock_t * lock)
{
	unlock (lock);

	// TODO: do not wake up all waiting tasks
	// but just one with top priority
	// wake_up_one (lock);

	wake_up ((struct wait_queue *) lock);
}

