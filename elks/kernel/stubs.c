#include <linuxmt/types.h>
#include <linuxmt/time.h>

/* struct timeval xtime; */

void kill_proc(){;}
/*void sys_kill(){;} */
void do_exit(){;}
void hard_reset_now()
{
#asm
	mov ax,#0x40            ! No memory check on reboot
	mov ds,ax               !
	mov [0x72],#0x1234      !

	jmp #0xffff:0
#endasm
}
