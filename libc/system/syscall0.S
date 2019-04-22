// Actual system calls
// Must be consistent with kernel system entry

	.code16

	.text

	.global _syscall_0
	.global _syscall_1
	.global _syscall_2
	.global _syscall_2va
	.global _syscall_3
	.global _syscall_4
	.global _syscall_5

_syscall_0:
#ifdef __IA16_CALLCVT_REGPARMCALL
_syscall_1:
	xchg	%ax,%bx
#endif
	int	$0x80

_syscall_test:
	test	%ax,%ax
	jns	_syscall_ok
	neg	%ax
	mov	%ax,errno
	mov	$-1,%ax

_syscall_ok:
	ret

#ifdef __IA16_CALLCVT_CDECL
_syscall_1:
	mov	%sp,%bx
	mov	2(%bx),%bx
	jmp	_syscall_0

_syscall_2:
	mov	%sp,%bx
	mov	4(%bx),%cx
	mov	2(%bx),%bx
	jmp	_syscall_0

_syscall_2va:
_syscall_3:
	mov	%sp,%bx
	mov	6(%bx),%dx
	mov	4(%bx),%cx
	mov	2(%bx),%bx
	jmp	_syscall_0

_syscall_4:
	mov	%sp,%bx
	push	%di
	mov	8(%bx),%di
	mov	6(%bx),%dx
	mov	4(%bx),%cx
	mov	2(%bx),%bx
	int	$0x80
	pop	%di
	jmp	_syscall_test

_syscall_5:
	mov	%sp,%bx
	push	%si
	mov	10(%bx),%si
	push	%di
	mov	8(%bx),%di
	mov	6(%bx),%dx
	mov	4(%bx),%cx
	mov	2(%bx),%bx
	int	$0x80
	pop	%di
	pop	%si
	jmp	_syscall_test
#elif defined __IA16_CALLCVT_STDCALL
_syscall_1:
	pop	%dx
	pop	%bx
	push	%dx
	jmp	_syscall_0

_syscall_2:
	pop	%dx
	pop	%bx
	pop	%cx
	push	%dx
	jmp	_syscall_0

_syscall_2va:
	mov	%sp,%bx
	mov	6(%bx),%dx
	mov	4(%bx),%cx
	mov	2(%bx),%bx
	jmp	_syscall_0

_syscall_3:
	mov	%sp,%bx
	mov	6(%bx),%dx
	mov	4(%bx),%cx
	mov	2(%bx),%bx
	int	$0x80
	test	%ax,%ax
	jns	0f
	neg	%ax
	mov	%ax,errno
	mov	$-1,%ax
0:
	ret	$6

_syscall_4:
	mov	%sp,%bx
	push	%di
	mov	8(%bx),%di
	mov	6(%bx),%dx
	mov	4(%bx),%cx
	mov	2(%bx),%bx
	int	$0x80
	pop	%di
	test	%ax,%ax
	jns	0f
	neg	%ax
	mov	%ax,errno
	mov	$-1,%ax
0:
	ret	$8

_syscall_5:
	mov	%sp,%bx
	push	%si
	mov	10(%bx),%si
	push	%di
	mov	8(%bx),%di
	mov	6(%bx),%dx
	mov	4(%bx),%cx
	mov	2(%bx),%bx
	int	$0x80
	pop	%di
	pop	%si
	test	%ax,%ax
	jns	0f
	neg	%ax
	mov	%ax,errno
	mov	$-1,%ax
0:
	ret	$10
#elif defined __IA16_CALLCVT_REGPARMCALL
_syscall_2:
_syscall_3:
	xchg	%dx,%cx
	jmp	_syscall_0

_syscall_2va:
	mov	%dx,%cx
	mov	%bx,%dx
	mov	%sp,%bx
	mov	2(%bx),%bx
	xchg	%bx,%dx
	jmp	_syscall_0

_syscall_4:
	xchg	%dx,%cx
	push	%di
	mov	%sp,%di
	mov	4(%di),%di
	int	$0x80
	pop	%di
	test	%ax,%ax
	jns	0f
	neg	%ax
	mov	%ax,errno
	mov	$-1,%ax
0:
	ret	$2

_syscall_5:
	xchg	%dx,%cx
	push	%si
	push	%di
	mov	%sp,%di
	mov	8(%di),%si
	mov	6(%di),%di
	int	$0x80
	pop	%di
	pop	%si
	test	%ax,%ax
	jns	0f
	neg	%ax
	mov	%ax,errno
	mov	$-1,%ax
0:
	ret	$4
#else
# error "unknown calling convention!"
#endif

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
#ifndef __IA16_CALLCVT_REGPARMCALL
	push %bx
#else
	mov %bx,%ax
#endif
	add %bx,%bx
	mov	_sigtable-2(%bx),%bx  // offset by 2 because no entry for signal 0
	call *%bx
#ifdef __IA16_CALLCVT_CDECL
	inc %sp
	inc %sp
#endif

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

//------------------------------------------------------------------------------

	.global breakpoint

breakpoint:

	int $3
	ret

//------------------------------------------------------------------------------

	.data

	.extern errno
	.extern _sigtable
