
#include <linuxmt/config.h>

#ifdef CONFIG_SIBO_CONSOLE_ECHO || CONFIG_SIBO_CONSOLE_SERIAL || CONFIG_SIBO_CHAR_SIBO_SERIAL

	.global _send_byte

;--------------------------------------------------------------

_send_byte:
	cli			;clear interrupts and turn off protection
	out 0x15, al		
	
	push bp

	mov bp, sp		;get parameters
	mov al, 4[bp]
	mov character, al
	pop bp

	in ax, #0x24		;turn on power to 'soap on a rope'
	or ax, #0x8000		;channel 5
	out 0x24, ax

	in ax, #0x2c		;enable clock to 'soap on a rope'
	or ax, #0x1000		;channel 5
	out 0x2c, ax

	in al, #0x8e		;select channel 5
	or al, #0x10
	out 0x8e, al

				;read port A to find out what's 
				;there on the selected channel
	mov al, #0x43
	out 0x8c, al
	in al, 0x8a
	test al,al
	jne continue		;found ASIC5 on the line....

	sti
	ret

continue:
	mov al, #0x82		;select Peripheral mode
	out 0x8c, al
	mov al, #0x05
	out 0x8a, al

	mov al, #0x87		;turn RS232, Drivers & enable
	out 0x8c, al
	mov al, #0x15
	out 0x8a, al

	mov al, #0x83		;assert CTS & DSR
	out 0x8c, al
	mov al, #0x06
	out 0x8a, al

	mov al, #0x8a		;baud LSB
	out 0x8c, al
	mov al, speed_lo
	out 0x8a, al

	mov al, #0x8b		;baud MSB
	out 0x8c,al
	mov al, speed_hi
	out 0x8a, al

	mov al, #0x88		;set mode (8 bits, 1 stop, 0 parity)
	out 0x8c, al
	mov al, #0x06
	out 0x8a, al

	mov al, #0x86		;disable all UART interrupts
	out 0x8c,al
	mov al, #00
	out 0x8a, al
	
	mov al, #0x89		;actually send byte
	out 0x8c, al
	mov al, character
	out 0x8a, al

waitdone:
	mov al, #0xC8		;wait till tx finished
	out 0x8c, al
	in al, 0x8a
	
	test al, #0x08
	jz waitdone		;loop until TX buf is empty
txfin:
	mov al, #0xC8
	out 0x8c, al
	in al, 0x8a
	
	test al, #0x10
	jnz txfin		;loop until TX is finished

	sti			;re-enable interupts
	ret

;-----------------------------------------------------------------
;-----------------------------------------------------------------

	.data
character:
	.byte 0x00
speed_lo:
	.byte 0xf7
speed_hi:
	.byte 0xff

#endif
