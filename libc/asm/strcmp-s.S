//------------------------------------------------------------------------------
// #include <string.h>
// int strcmp(const char *s1, const char *s2)
//------------------------------------------------------------------------------

#include <libc-private/call-cvt.h>

        .code16
        .text

        .global strcmp
strcmp:
        push %bp
        mov %sp,%bp

#ifndef __IA16_CALLCVT_REGPARMCALL
        mov %si,%dx             // save SI DI ES -> DX BX CX
        mov %di,%bx
        mov %es,%cx
        mov %ds,%ax
        mov %ax,%es
#else
	push %si
	mov %di,%bx
	mov %es,%cx
	mov %ds,%si
	mov %si,%es
#endif

#ifndef __IA16_CALLCVT_REGPARMCALL
        mov 4+FAR_ADJ_(%bp),%di // s1
        mov 6+FAR_ADJ_(%bp),%si // s2
#else
	mov %ax,%di // s1
	mov %dx,%si // s2
#endif
        cld
1:
        lodsb
        scasb
        jne 2f                  // if bytes different skip out
        test %al,%al
        jne 1b                  // if this byte NUL then strings equal
        xor %ax,%ax             // so return zero
        jmp 3f
2:
        cmc
        sbb %ax,%ax             // return correct val (-1,1)
        orb $1,%al
3:
#ifndef __IA16_CALLCVT_REGPARMCALL
        mov %dx,%si             // restore SI DI ES
        mov %bx,%di
        mov %cx,%es
#else
	pop %si
	mov %bx,%di
	mov %cx,%es
#endif

        pop %bp
        RET_(2)

//------------------------------------------------------------------------------
