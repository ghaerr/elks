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
