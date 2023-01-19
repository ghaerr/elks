/* ia16-elf-gcc ASM instrumentation functions for ELKS */
/* June 2022 Greg Haerr */

        .code16
        .text

        .global _get_csbyte
// int _get_csbyte(char *addr)
// return byte at CS:addr
_get_csbyte:
        push %bp
        mov  %sp,%bp
        mov  4(%bp),%bx
        mov  %cs:(%bx),%al
        xor  %ah,%ah
        pop  %bp
        ret

        .global _get_cs
// int _get_cs(void)
// return CS
_get_cs:
        push %cs
        pop  %ax
        ret

        .global _get_rdtsc
// unsigned long long _get_rdtsc(void)
// 386+ only, reads CPU time stamp counter
_get_rdtsc:
        mov  %sp,%bx
        mov  2(%bx),%bx // get address for 64-bit return
        rdtsc           // returns 64-bit EDX:EAX
        mov  %ax,(%bx)
        shr  $16,%eax
        mov  %ax,2(%bx)
        mov  %dx,4(%bx)
        shr  $16,%edx
        mov  %dx,6(%bx)
        ret
