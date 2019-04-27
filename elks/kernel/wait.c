
#include <linuxmt/kernel.h>
#include <linuxmt/sched.h>
#include <linuxmt/wait.h>

// Wait for event (actually a condition change)
// Returns 0 if true condition or 1 if interrupted

// Replaced the Linux wait queue by a simple condition check
// because cost of updating the queues on wait completion
// is more that traversing the ELKS limited list of tasks
// and multiple conditions can be handled with indirection

// More modern wait function than deprecated 'sleep_on'
// See issue #222

bool_t _wait_event (cond_t * c, bool_t i)
{
	bool_t res = 0;

	if (!cond_test (c)) {

		if (current == &task [0])
			panic ("task 0 waits condition %x", (int) c);

		wait_set ((struct wait_queue *) c);
		while (1) {
			current->state = i ? TASK_INTERRUPTIBLE : TASK_UNINTERRUPTIBLE;
			if (cond_test (c)) break;
			schedule ();

			if (i && current->signal)
			{
				// Interrupted by signal
				res = 1;
				break;
			}
		}

		current->state = TASK_RUNNING;
		wait_clear ((struct wait_queue *) c);
	}

	return res;
}
