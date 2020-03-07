//------------------------------------------------------------------------------
// ELKS boot probe
//------------------------------------------------------------------------------

#include <autoconf.h>

#include "boot_err.h"

	.code16

	.text

	.global load_prog

load_prog:

	push %es
	push %di

	mov drive_num,%dl
	mov $8,%ah
	int $0x13

	pop %di
	pop %es

	jnc 1f

	mov $msg_drive_get,%bx
	call _puts
	jmp _reboot

1:	mov %dh,%al  // maximum head (base 0)
	inc %ax
	mov %al,head_max

	mov %ch,%al  // maximum track (base 0)
	inc %ax
	mov %al,track_max

	mov %cl,%al  // maximum sector (base 1)
	and $0x3F,%al
	mov %al,sect_max

	mov $msg_track,%bx
	call _puts
	mov track_max,%al
	call _byte_hex

	mov $msg_head,%bx
	call _puts
	mov head_max,%al
	call _byte_hex

	mov $msg_sector,%bx
	call _puts
	mov sect_max,%al
	call _byte_hex

	mov $msg_success,%bx
	call _puts
	jmp _reboot

//------------------------------------------------------------------------------

_half_hex:

	and $0x0F,%al
	add $'0',%al
	cmp $'9',%al
	jle 1f
	add $('A'-'9'-1),%al
1:	jmp _putc

_byte_hex:

	mov %al,%dl

	mov $4,%cl
	shr %cl,%al
	call _half_hex
	mov %dl,%al
	jmp _half_hex

_word_hex:

	push %ax
	mov %ah,%al
	call _byte_hex
	pop %ax
	jmp _byte_hex

//------------------------------------------------------------------------------

msg_drive_get:
	.asciz "drive_get!\r\n"

msg_track:
	.asciz "C=0x"

msg_head:
	.asciz " H=0x"

msg_sector:
	.asciz " S=0x"

msg_success:
	.asciz "\r\n"

//------------------------------------------------------------------------------

