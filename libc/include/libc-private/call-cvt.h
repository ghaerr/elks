/* Internal macros to bridge differences between various calling conventions */

#ifndef _LIBC_PRIVATE_CALL_CVT_H
#define _LIBC_PRIVATE_CALL_CVT_H

#if defined __MEDIUM__ || defined __LARGE__ || defined __HUGE__
/* gcc-ia16 should define __IA16_CMODEL_IS_FAR_TEXT if the memory model uses
 * far code addresses; define it ourselves in case it is missing */
# ifndef __IA16_CMODEL_IS_FAR_TEXT
#   define __IA16_CMODEL_IS_FAR_TEXT
# endif
/* Adjustment to stack frame offsets to account for far return addresses */
# define FAR_ADJ_	2
/* How to return from a subroutine with N bytes of arguments */
# if defined __IA16_CALLCVT_STDCALL
#   define RET_(n)	.if (n); lret $(n); .else; lret; .endif
# elif defined __IA16_CALLCVT_CDECL
#   define RET_(n)	lret
# elif defined __IA16_CALLCVT_REGPARMCALL
#   define RET_(n)	.if (n)>6; lret $(n)-6; .else; lret; .endif
# else
#   error "unknown calling convention!"
# endif
# define RET_VA_	lret
/* How to call a subroutine of the same code far-ness in the (same) near text
   segment */
# define CALL_N_(s)	pushw %cs; call s
/* How to call a subroutine which may be in a different text segment */
# ifdef __IA16_ABI_SEGELF
/* FIXME: make this neater... */
#   define CALL__(s, a)	.set __tmp, .+3; \
			.reloc __tmp, R_386_SEG16, #a; \
			lcall $0, $(s)
#   define CALL_(s)	CALL__(s, s##!)
#   define JMP__(s, a)	.set __tmp, .+3; \
			.reloc __tmp, R_386_SEG16, #a; \
			ljmp $0, $(s)
#   define JMP_(s)	JMP__(s, s##!)
# else
#   define CALL_(s)	.reloc .+3, R_386_OZSEG16, (s); lcall $0, $(s)
#   define JMP_(s)	.reloc .+3, R_386_OZSEG16, (s); ljmp $0, $(s)
# endif
#else  /* not medium, large, or huge model */
# undef __IA16_CMODEL_IS_FAR_TEXT
# define FAR_ADJ_	0
# if defined __IA16_CALLCVT_STDCALL
#   define RET_(n)	.if (n); ret $(n); .else; ret; .endif
# elif defined __IA16_CALLCVT_CDECL
#   define RET_(n)	ret
# elif defined __IA16_CALLCVT_REGPARMCALL
#   define RET_(n)	.if (n)>6; ret $(n)-6; .else; ret; .endif
# else
#   error "unknown calling convention!"
# endif
# define RET_VA_	ret
# define CALL_N_(s)	call s
# define CALL_(s)	call s
# define JMP_(s)	jmp s
#endif  /* not medium, large, or huge model */

#endif
