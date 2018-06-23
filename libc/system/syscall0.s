// Actual system calls
// Must be consistent with kernel system entry

	.code16

	.text

	.global _syscall_0
	.global _syscall_1
	.global _syscall_2
	.global _syscall_3
	.global _syscall_4
	.global _syscall_5

_syscall_0:
	int    $0x80

_syscall_test:
	test   %ax,%ax
	jge    _syscall_ok
	neg    %ax
	mov    %ax,errno
	mov    $-1,%ax

_syscall_ok:
	ret

_syscall_1:
	mov    %sp,%bx
	mov    2(%bx),%bx
	jmp    _syscall_0

_syscall_2:
	mov    %sp,%bx
	mov    4(%bx),%cx
	mov    2(%bx),%bx
	jmp    _syscall_0

_syscall_3:
	mov    %sp,%bx
	mov    6(%bx),%dx
	mov    4(%bx),%cx
	mov    2(%bx),%bx
	jmp    _syscall_0

_syscall_4:
	mov    %sp,%bx
	push   %di
	mov    8(%bx),%di
	mov    6(%bx),%dx
	mov    4(%bx),%cx
	mov    2(%bx),%bx
	int    $0x80
	pop    %di
	jmp    _syscall_test

_syscall_5:
	mov    %sp,%bx
	push   %si
	mov    10(%bx),%si
	push   %di
	mov    8(%bx),%di
	mov    6(%bx),%dx
	mov    4(%bx),%cx
	mov    2(%bx),%bx
	int    $0x80
	pop    %di
	pop    %si
	jmp    _syscall_test

//-----------------------------------------------------------------------------

// Signal callback from kernel

	.global _syscall_signal

_syscall_signal:
	push %bp
	mov %sp,%bp

	pushf
	push %ax
	push %bx
	push %cx
	push %dx
	push %si
	push %di
	push %es

	mov 4(%bp),%bx
	push %bx
	add %bx,%bx
	mov	_sigtable-2(%bx),%bx  // offset by 2 because no entry for signal 0
	call *%bx
	inc %sp
	inc %sp

	pop %es
	pop %di
	pop %si
	pop %dx
	pop %cx
	pop %bx
	pop %ax
	popf

	pop %bp
	ret $2  // get rid of the signum

//-----------------------------------------------------------------------------

	.data

	.extern errno
	.extern _sigtable
