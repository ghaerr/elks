// Actual system calls
// Must be consistent with kernel system entry

#include <libc-private/call-cvt.h>

	.arch	i8086, nojumps
	.code16

	.text

	.global _syscall_0
	.global _syscall_1
	.global _syscall_2
	.global _syscall_2p
	.global _syscall_3
	.global _syscall_4
	.global _syscall_5

#ifndef __IA16_CALLCVT_REGPARMCALL
_syscall_0:
	int    $0x80

_syscall_test:
	test   %ax,%ax
	jns    _syscall_ok
	neg    %ax
	mov    %ax,errno
	mov    $-1,%ax

_syscall_ok:
	RET_(0)
#endif

#if defined __IA16_CALLCVT_CDECL
_syscall_1:
	mov    %sp,%bx
	mov    2+FAR_ADJ_(%bx),%bx
	jmp    _syscall_0

_syscall_2:
	mov    %sp,%bx
	mov    4+FAR_ADJ_(%bx),%cx
	mov    2+FAR_ADJ_(%bx),%bx
	jmp    _syscall_0

_syscall_3:
_syscall_2p:
	mov    %sp,%bx
	mov    6+FAR_ADJ_(%bx),%dx
	mov    4+FAR_ADJ_(%bx),%cx
	mov    2+FAR_ADJ_(%bx),%bx
	jmp    _syscall_0

_syscall_4:
	mov    %sp,%bx
	push   %di
	mov    8+FAR_ADJ_(%bx),%di
	mov    6+FAR_ADJ_(%bx),%dx
	mov    4+FAR_ADJ_(%bx),%cx
	mov    2+FAR_ADJ_(%bx),%bx
	int    $0x80
	pop    %di
	jmp    _syscall_test

_syscall_5:
	mov    %sp,%bx
	push   %si
	mov    10+FAR_ADJ_(%bx),%si
	push   %di
	mov    8+FAR_ADJ_(%bx),%di
	mov    6+FAR_ADJ_(%bx),%dx
	mov    4+FAR_ADJ_(%bx),%cx
	mov    2+FAR_ADJ_(%bx),%bx
	int    $0x80
	pop    %di
	pop    %si
	jmp    _syscall_test
#elif defined __IA16_CALLCVT_STDCALL
_syscall_1:
# ifdef __IA16_CMODEL_IS_FAR_TEXT
	pop %dx
	pop %cx
	pop %bx
	push %cx
	push %dx
# else
	pop %dx
	pop %bx
	push %dx
# endif
	jmp _syscall_0

_syscall_2:
# ifdef __IA16_CMODEL_IS_FAR_TEXT
	mov %sp,%bx
	mov 6(%bx),%cx
	mov 4(%bx),%bx
	pushw %cs
	call _syscall_0
	RET_(4)
# else
	pop %dx
	pop %bx
	pop %cx
	push %dx
	jmp _syscall_0
# endif

_syscall_2p:
	/* variadic function, callee must not pop any arguments */
	mov %sp,%bx
	mov 6+FAR_ADJ_(%bx),%dx
	mov 4+FAR_ADJ_(%bx),%cx
	mov 2+FAR_ADJ_(%bx),%bx
	jmp _syscall_0

_syscall_3:
	mov %sp,%bx
	mov 6+FAR_ADJ_(%bx),%dx
	mov 4+FAR_ADJ_(%bx),%cx
	mov 2+FAR_ADJ_(%bx),%bx
	CALL_N_(_syscall_0)
	RET_(6)

_syscall_4:
	mov %sp,%bx
	push %di
	mov 8+FAR_ADJ_(%bx),%di
	mov 6+FAR_ADJ_(%bx),%dx
	mov 4+FAR_ADJ_(%bx),%cx
	mov 2+FAR_ADJ_(%bx),%bx
	CALL_N_(_syscall_0)
	pop %di
	RET_(8)

_syscall_5:
	mov %sp,%bx
	push %si
	mov 10+FAR_ADJ_(%bx),%si
	push %di
	mov 8+FAR_ADJ_(%bx),%di
	mov 6+FAR_ADJ_(%bx),%dx
	mov 4+FAR_ADJ_(%bx),%cx
	mov 2+FAR_ADJ_(%bx),%bx
	CALL_N_(_syscall_0)
	pop %di
	pop %si
	RET_(10)
#elif defined __IA16_CALLCVT_REGPARMCALL
_syscall_2p:
	push %bp
	mov %sp,%bp
	mov 4+FAR_ADJ_(%bp),%cx
	pop %bp
_syscall_2:
_syscall_3:
	xchg %cx,%dx
_syscall_1:
_syscall_0:
	xchg %ax,%bx

_syscall:
	int    $0x80

_syscall_test:
	test   %ax,%ax
	jns    _syscall_ok
	neg    %ax
	mov    %ax,errno
	mov    $-1,%ax

_syscall_ok:
	RET_(0)

_syscall_4:
	push %di
	mov %sp,%di
	mov 4+FAR_ADJ_(%di),%di
	xchg %cx,%dx
	xchg %ax,%bx
	CALL_N_(_syscall)
	pop %di
	RET_(8)

_syscall_5:
	push %di
	mov %sp,%di
	push %si
	mov 6+FAR_ADJ_(%di),%si
	mov 4+FAR_ADJ_(%di),%di
	xchg %cx,%dx
	xchg %ax,%bx
	CALL_N_(_syscall)
	pop %si
	pop %di
	RET_(10)
#else
# error "unknown calling convention"
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

	mov 6(%bp),%bx
#ifndef __IA16_CALLCVT_REGPARMCALL
	push %bx
#else
	mov %bx,%ax
#endif
#ifdef __IA16_CMODEL_IS_FAR_TEXT
	add %bx,%bx
	add %bx,%bx
	lcallw *_sigtable-4(%bx)  // offset by 4 because no entry for signal 0
#else
	add %bx,%bx
	mov	_sigtable-2(%bx),%bx  // offset by 2 because no entry for signal 0
	call *%bx
#endif
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
	lret $2  // get rid of the signum

//------------------------------------------------------------------------------

	.global breakpoint

breakpoint:

	int $3
	RET_(0)

//------------------------------------------------------------------------------

	.data

	.extern errno
	.extern _sigtable
