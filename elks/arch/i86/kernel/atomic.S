// Atomic primitives
// Architecture dependent

	.code16

	.text

// void count_up (count_t * count)

	.global count_up

// Atomic LOCK INC instruction

count_up:

	mov %sp,%bx
	mov 2(%bx),%bx
	lock
	incw (%bx)
	ret

// void count_down (count_t * count)

	.global count_down

// Atomic LOCK DEC instruction

count_down:

	mov %sp,%bx
	mov 2(%bx),%bx
	lock
	decw (%bx)
	ret

// lock_t try_lock (lock_t * lock)

	.global try_lock

// Atomic LOCK XCHG instruction
// AX contains previous value after exchange
// so not acquired if AX not zero

try_lock:

	mov %sp,%bx
	mov 2(%bx),%bx
	mov $1,%ax
	lock
	xchg (%bx),%ax
	ret

// void unlock (lock_t * lock)

	.global unlock

unlock:

	mov %sp,%bx
	mov 2(%bx),%bx
	lock
	movw $0,(%bx)
	ret
