// conio API to BIOS for BIOS or Headless Console
//
// INT 16h AH=00h, INT 16H AH=01h (keyboard)
// INT 10h AH=0Eh (write teletype)
//
#include <linuxmt/config.h>
	.arch	i8086, nojumps
	.code16
	.text

	.global conio_init
	.global conio_poll
	.global	conio_putc

// void conio_init(void)

conio_init:
	ret

// int conio_poll
// INT 16h AH=00h (read kbd)
// INT 16h AH=01h (get kbd status)
// returns scan code in AH, ASCII char in AL

conio_poll:
#ifdef CONFIG_HW_MK88
	int    $0x0b		// interrogate MK-88 keyboard before INT 16h
#endif
	mov    $1,%ah		// get kbd status
	int    $0x16
	jnz    1f		// key pressed
	xor    %ax,%ax
1:	or     %ax,%ax
	jz     9f
	xor    %ah,%ah		// read kbd scan/char
	int    $0x16
9:	ret

// void conio_putc (byte_t ch)
// INT 10h BIOS AH=0Eh (write teletype)

conio_putc:
	push	%bp		// some BIOS destroy BP
	mov	%sp,%bp
	mov	4(%bp),%al
	mov	$0x0E,%ah	// write text in teletype mode
	mov	$0x0007,%bx	// page 0 fg color 7
	int	$0x10
	pop	%bp
	ret
