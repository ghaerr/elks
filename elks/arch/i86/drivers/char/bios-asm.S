// BIOS routines for Headless Console
//
// INT 16h functions 0,1 (keyboard)
// INT 10h function 0x0E (write teletype)
//
	.arch	i8086, nojumps
	.code16
	.text

	.global bios_kbd_poll
	.global	bios_textout

// int bios_kbd_poll
// uses INT 16h BIOS functions 0 and 1
// returns scan code in AH, ASCII char in AL

bios_kbd_poll:
	mov    $1,%ah		// get kbd status
	int    $0x16
	jnz    1f		// key pressed
	xor    %ax,%ax
1:	or     %ax,%ax
	jz     9f
	xor    %ah,%ah		// read kbd scan/char
	int    $0x16
9:	ret

// void bios_textout (byte_t ch)
// uses INT 10h BIOS function 0x0E

bios_textout:
	push	%bp		// some BIOS destroy BP
	mov	%sp,%bp
	mov	4(%bp),%al
	mov	$0x0E,%ah	// write text in teletype mode
	mov	$0x0007,%bx	// page 0 fg color 7
	int	$0x10
	pop	%bp
	ret
