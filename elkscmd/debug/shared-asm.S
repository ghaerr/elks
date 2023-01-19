/* ia16-elf-gcc ASM instrumentation functions for ELKS */
/* June 2022 Greg Haerr */

        .code16
        .text

        .global getcsbyte
// int getcsbyte(char *addr)
// return byte at CS:addr
getcsbyte:
        push %bp
        mov  %sp,%bp
        mov  4(%bp),%bx
        mov  %cs:(%bx),%al
        xor  %ah,%ah
        pop  %bp
        ret

        .global getcs
// int getcs(void)
// return CS
getcs:
        push %cs
        pop  %ax
        ret

        .global rdtsc
// unsigned long long rdtsc(void)
// 386+ only, reads CPU time stamp counter
rdtsc:
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
