// Atomic primitives
// Architecture dependent

	.code16

	.text

// void atomic_inc (atomic_t * count)

	.global atomic_inc

// Atomic LOCK INC instruction

atomic_inc:

	mov %sp,%bx
	mov 2(%bx),%bx
	lock
	incw (%bx)
	ret

// void atomic_dec (atomic_t * count)

	.global atomic_dec

// Atomic LOCK DEC instruction

atomic_dec:

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
