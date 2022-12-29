// Actual system calls
// Must be consistent with kernel system entry

#include <libc-private/call-cvt.h>

	.arch	i8086, nojumps
	.code16

	.text

#ifndef __IA16_CALLCVT_REGPARMCALL
#ifdef L_sys01
	.global _syscall_0
_syscall_0:
	int    $0x80

	.global _syscall_test
_syscall_test:
	test   %ax,%ax
	jns    _syscall_ok
	neg    %ax
	mov    %ax,errno
	mov    $-1,%ax

_syscall_ok:
	RET_(0)
#endif
#endif

#if defined __IA16_CALLCVT_CDECL
#ifdef L_sys01
	.global _syscall_1
_syscall_1:
	mov    %sp,%bx
	mov    2+FAR_ADJ_(%bx),%bx
	jmp    _syscall_0
#endif

#ifdef L_sys23
	.global _syscall_2
_syscall_2:
	mov    %sp,%bx
	mov    4+FAR_ADJ_(%bx),%cx
	mov    2+FAR_ADJ_(%bx),%bx
	jmp    _syscall_0

	.global _syscall_2p
	.global _syscall_3
_syscall_3:
_syscall_2p:
	mov    %sp,%bx
	mov    6+FAR_ADJ_(%bx),%dx
	mov    4+FAR_ADJ_(%bx),%cx
	mov    2+FAR_ADJ_(%bx),%bx
	jmp    _syscall_0
#endif

#ifdef L_sys4
	.global _syscall_4
	.global _syscall_test
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
#endif

#ifdef L_sys5
	.global _syscall_5
	.global _syscall_test
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
#endif

#elif defined __IA16_CALLCVT_STDCALL

#ifdef L_sys01
	.global _syscall_1
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
#endif

#ifdef L_sys23
	.global _syscall_2
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

	.global _syscall_2p
_syscall_2p:
	/* variadic function, callee must not pop any arguments */
	mov %sp,%bx
	mov 6+FAR_ADJ_(%bx),%dx
	mov 4+FAR_ADJ_(%bx),%cx
	mov 2+FAR_ADJ_(%bx),%bx
	jmp _syscall_0

	.global _syscall_3
_syscall_3:
	mov %sp,%bx
	mov 6+FAR_ADJ_(%bx),%dx
	mov 4+FAR_ADJ_(%bx),%cx
	mov 2+FAR_ADJ_(%bx),%bx
	CALL_N_(_syscall_0)
	RET_(6)
#endif

#ifdef L_sys4
	.global _syscall_4
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
#endif

#ifdef L_sys5
	.global _syscall_5
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
#endif

#elif defined __IA16_CALLCVT_REGPARMCALL

#ifdef L_sys23
	.global _syscall_2p
_syscall_2p:
	push %bp
	mov %sp,%bp
	mov 4+FAR_ADJ_(%bp),%cx
	pop %bp

	.global _syscall_2
	.global _syscall_3
_syscall_2:
_syscall_3:
	xchg %cx,%dx
	jmp _syscall_0
#endif

#ifdef L_sys01
	.global _syscall_0
	.global _syscall_1
_syscall_1:
_syscall_0:
	xchg %ax,%bx

	.global _syscall
_syscall:
	int    $0x80

	.global _syscall_test
_syscall_test:
	test   %ax,%ax
	jns    _syscall_ok
	neg    %ax
	mov    %ax,errno
	mov    $-1,%ax

_syscall_ok:
	RET_(0)
#endif

#ifdef L_sys4
	.global _syscall_4
	.global _syscall
_syscall_4:
	push %di
	mov %sp,%di
	mov 4+FAR_ADJ_(%di),%di
	xchg %cx,%dx
	xchg %ax,%bx
	CALL_N_(_syscall)
	pop %di
	RET_(8)
#endif

#ifdef L_sys5
	.global _syscall_5
	.global _syscall
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
#endif

#else
# error "unknown calling convention"
#endif

	.data
	.extern errno
