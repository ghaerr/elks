#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/wait.h>
#include <linuxmt/sched.h>
#include <linuxmt/config.h>
#include <linuxmt/fs.h> /* for ROOT_DEV */

#include <arch/segment.h>

__arch_mminit arch_segs;
int arch_cpu;			/* Processor type */
extern long int basmem;

/* Stubs for functions needed elsewhere */

void hard_reset_now(void)
{
#ifndef S_SPLINT_S
#asm
	mov ax,#0x40		! No memory check on reboot
	mov ds, ax
	mov [0x72],#0x1234
	jmp #0xffff:0

#endasm
#endif
}

void setup_arch(seg_t *start, seg_t *end)
{
#ifndef S_SPLINT_S
/*
 *	Save segments
 */
#asm
	mov bx, #_arch_segs
        mov [bx], cs
	mov [bx+2], di
        mov [bx+8], ss
	mov [bx+6], si

!	This is out of order to save a segment load and a few bytes :)

        mov [bx+4], ds
	mov [bx+10], dx

!	mov ds, ax

#endasm
#endif

    arch_segs.lowss = arch_segs.endss;

#ifdef CONFIG_COMPAQ_FAST

/*
 *	Switch COMPAQ Deskpto to high speed
 */

    outb_p(1,0xcf);

#endif

/*
 *	Fill in the MM numbers - really ought to be in mm not kernel ?
 */

#ifndef CONFIG_ARCH_SIBO	

    *end = (seg_t)(setupw(0x2a) << 6 - RAM_REDUCE);

    /* XXX plac: free root ram disk */

    *start = get_ds();
    *start += ((unsigned int) (_endbss+15)) >> 4;

#else

    *end = (basmem)<<6;
    *start = get_ds();
    *start += (unsigned int) 0x1000;

#endif

    ROOT_DEV = setupw(0x1fc);

    arch_cpu = setupb(0x20);

}

#ifndef S_SPLINT_S
#asm

	export _sys_dlload

_sys_dlload:

#ifndef CONFIG_SOCKET

	export _sys_socket

_sys_socket:

	export _sys_bind

_sys_bind:

	export _sys_listen

_sys_listen:

	export _sys_accept

_sys_accept:

	export _sys_connect

_sys_connect:

#endif

	export _no_syscall

_no_syscall:
	mov	ax,#-38
	ret

#endasm
#endif
