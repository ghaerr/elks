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

        mov %si,%dx             // save SI DI ES -> DX BX CX
        mov %di,%bx
        mov %es,%cx
        mov %ds,%ax
        mov %ax,%es

        mov 4+FAR_ADJ_(%bp),%di // s1
        mov 6+FAR_ADJ_(%bp),%si // s2
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
        mov %dx,%si             // restore SI DI ES
        mov %bx,%di
        mov %cx,%es

        pop %bp
        RET_(2)

//------------------------------------------------------------------------------
