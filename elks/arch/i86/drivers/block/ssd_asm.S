
#include <linuxmt/config.h>

#ifdef CONFIG_BLK_DEV_SSD

; Small program to prove that we can drive the serial port directly
; write a string to tell yo are up and running and the echo back 
; what is typed, until the '_' is typed.

; SDW 3/3/99

	.text
	.global _ssd_open4
	.global _ssd_close4
	.global _ssd_read4
	.global _ssd_readblk4
	.global _ssd_write4
	.global _ssd_erase4

;----------------------------------------------------------------------
; Open a connect to ASIC4 and report the device/size information
;

_ssd_open4:
	cli			;clear interrupts and turn off protection
	out 0x15, al		

	in ax, #0x24		;turn on power to packs
	or ax, #0x8000
	out 0x24, ax

	in ax, #0x2C		;enable clock to SSD
	or ax, #0x1000
	out 0x2C, ax

	push bp			;get 'channel parameter'
	mov bp, sp
	mov ah, 4[bp]
	pop bp

	in al, #0x8E		;select channel
	and al, #0xE0
	or al, #0x60		;select fast clock
	or al, ah
	out 0x8E, al

	mov al, #0x46		;inquire for ASIC4

	out 0x8c, al
	in al, 0x8a
	test al,al
	je open4_error		;not found....

	mov al, #0xC0		;read configuration of ASIC4
	out 0x8C, al
	in al, #0x8A

	mov bl, #0x23 		;fix as intel type 1 1x128K device

	mov al, #0x81		;auto-set device size (upto 2M)
	out 0x8C, al
	mov al, bl
	and al, #0x07
	out 0x8A, al
	
	mov al, bl		;return OK
	sti
	ret

_ssd_close4:
	cli			;clear interrupts and turn off protection
	out 0x15, al		

open4_error:
	in ax, #0x2c		;disable clock to 'soap on a rope'
	and ax, #0xEFFF
	out 0x2c, ax

	in ax, #0x24		;turn off power to packs
	and ax, #0x7FFF
	out 0x24, ax

	mov ax,#0x0000		;Return Error!!!!
	sti
	ret

;------------------------------------------------------------------
; read a memory location from ASIC 4 (assumes 1*128K Device
;
; char ssd_read4( int high_address, int low_address)

_ssd_read4:
	cli			;clear interrupts and turn off protection
	out 0x15, al		

	push bp			;get parameters
	mov bp, sp
	mov ax, [bp + 4]
	mov ssd_address_high, ax
	mov ax, [bp + 6]
	mov ssd_address_low, ax
	pop bp

	mov ax, #0x93		;set address multiple write
	out 0x8C, al
	mov ax, ssd_address_low
	out 0x8A, al
	mov al, ah
	out 0x8A, al
	mov ax, ssd_address_high
	out 0x8A, al
	mov al, ah
	out 0x8A, al

	mov ax, #0x87		;set control register to OE
	out 0x8C, al
	mov al, #0x01
	out 0x8A, al

	mov ax, #0xC0		;read data
	out 0x8C, al
	in al, #0x8A

	mov ah, #0x01
	sti
	ret

;------------------------------------------------------------------
; read a memory location from ASIC 4 (assumes 1*128K Device
;
; ssd_read4( int high_address, int low_address, char *data)

_ssd_readblk4:
	cli			;clear interrupts and turn off protection
	out 0x15, al		

	push bp			;get parameters
	mov bp, sp
	mov ax, [bp+4]
	mov ssd_address_high, ax
	mov ax, [bp+6]
	and ax, #0xFFF0		;make sure it's the start of a 16 byte block
	mov ssd_address_low, ax
	mov ax, [bp+8]
	mov ssd_data, ax
	pop bp

	mov ax, #0x93		;set address multiple write
	out 0x8C, al
	mov ax, ssd_address_low
	out 0x8A, al
	mov al, ah
	out 0x8A, al
	mov ax, ssd_address_high
	out 0x8A, al
	mov al, ah
	out 0x8A, al

	cld
	mov di, ssd_data
	mov cx, #0x0010

readblk_loop:
	mov ax, #0x87		;set control register to OE
	out 0x8C, al
	mov al, #0x01
	out 0x8A, al

	mov ax, #0xC0		;read data
	out 0x8C, al
	in al, #0x8A

	stosb

	mov ax, #0x82		;increase address
	out 0x8C, al
	out 0x8A, al
	
	loop readblk_loop

	mov ah, #0x01
	sti
	ret
	
;------------------------------------------------------------------
; write a memory location from ASIC 4
;
; int ssd_write( int high_address, int low_address, char data)

_ssd_write4:
	cli			;clear interrupts and turn off protection
	out 0x15, al		

	push bp			;get parameters
	mov bp, sp
	mov ax, [bp + 4]
	mov ssd_address_high, ax
	mov ax, [bp + 6]
	mov ssd_address_low, ax
	mov ax, [bp + 8]
	mov ssd_data, ax
	pop bp
				;power and clock already on
				;chanel and size selected
	mov ax, #0x93		;set address
	out 0x8C, al
	mov ax, ssd_address_low
	out 0x8A, al
	mov al, ah
	out 0x8A, al
	mov ax, ssd_address_high
	out 0x8A, al
	mov al, ah
	out 0x8A, al

	mov ax, #0x87		;set VPP
	out 0x8C, al
	mov al, #0x10
	out 0x8A, al

	mov cx, #0x0200		;delay (found by experiment)
loop1:
	nop
	loop loop1

	mov ax, #0x80		;write control register 'program setup'
	out 0x8C, al
	mov al, #0x40
	out #0x8A, al
	
	mov ax, #0x87		;set VPP + WR
	out 0x8C, al
	mov al, #0x12
	out 0x8A, al

	mov ax, #0x80		;write data
	out 0x8C, al
	mov al, ssd_data
	out #0x8A, al

	mov ax, #0x87		;set VPP + WR
	out 0x8C, al
	mov al, #0x12
	out 0x8A, al

	mov cx, #0x0050		;delay approx 20us
loop2:
	nop
	loop loop2

	mov ax, #0x80		;write control register 'verify setup'
	out 0x8C, al
	mov al, #0xC0
	out 0x8A, al
	
	mov ax, #0x87		;set VPP + WR
	out 0x8C, al
	mov al, #0x12
	out 0x8A, al

	mov cx, #0x0050		;delay approx 20us
loop3:
	nop
	loop loop3

	mov ax, #0x87		;set VPP + OE
	out 0x8C, al
	mov al, #0x11
	out 0x8A, al

	mov ax, #0xC0		;verify data
	out 0x8C, al
	in al, #0x8A

	mov bx, ax
	jmp ssd_write_ok

ssd_write_error:
	mov bx, #0xFFFF

ssd_write_ok:
	mov ax, #0x87		;clear VPP + OE
	out 0x8C, al
	mov al, #0x00
	out 0x8A, al
	
	mov ax, bx
	sti
	ret
	
;------------------------------------------------------------------
; erase the whole of flash ASIC 4 (assumes 1*128K Device)
;
; int ssd_erase4( int high_address)

_ssd_erase4:
	cli			;clear interrupts and turn off protection
	out 0x15, al		

	push bp			;get parameters
	mov bp, sp
	mov ax, [bp + 4]	;select block to erase
	mov ssd_address_high, ax
	mov ax, #0xffff		;scan 65536 address to confirm erase
	mov ssd_address_low, ax
	mov ax, #0x0008		;safety count (only try erase 1000 times)
	pop bp

				;the power and clocks should already be on
				;and the approriate chanel/size selected.
				;we don't even need to check it's really ASIC4
	mov ax, #0x93		;set address
	out 0x8C, al
	mov ax, ssd_address_low
	out 0x8A, al
	mov al, ah
	out 0x8A, al
	mov ax, ssd_address_high
	out 0x8A, al
	mov al, ah
	out 0x8A, al

	mov ax, #0x87		;set VPP
	out 0x8C, al
	mov al, #0x10
	out 0x8A, al

	mov cx, #0x0200		;delay (found by experiment)
loop4:
	nop
	loop loop4

ssd_erase_start:
	mov ax, ssd_data	;used for safety count
	dec ax
	mov ssd_data, ax

	cmp ax, #0x0000
	jne ssd_erase_allowed

	mov bx, #0xFFFF		;tried too many times

	mov ax, #0x87		;clear VPP + OE
	out 0x8C, al
	mov al, #0x00
	out 0x8A, al
	
	in ax, #0x2c		;disable clock to 'soap on a rope'
	and ax, #0xEFFF
	out 0x2c, ax

	in ax, #0x24		;turn off power to packs
	and ax, #0x7FFF
	out 0x24, ax

	mov ax, bx
	sti
	ret
	
ssd_erase_allowed:
	mov ax, #0x80		;write control register 'erase setup'
	out 0x8C, al
	mov al, #0x20
	out #0x8A, al
	
	mov ax, #0x87		;set VPP + WR
	out 0x8C, al
	mov al, #0x12
	out 0x8A, al

	mov ax, #0x80		;write control register 'erase'
	out 0x8C, al
	mov al, #0x20
	out #0x8A, al

	mov ax, #0x87		;set VPP + WR
	out 0x8C, al
	mov al, #0x12
	out 0x8A, al

	mov cx, #0x7F00		;delay approx 10ms
	
	sti
loop5:
	nop
	loop loop5
	cli
	out 0x15, al

	mov cx, ssd_address_low
	jmp ssd_erase_check

hop_skip_jump:
	jmp ssd_erase_start

ssd_erase_check:
	mov ssd_address_low, cx
	mov ax, #0x93		;set address (only scans the first 65536 locations)
	out 0x8C, al
	mov ax, cx
	out 0x8A, al
	mov al, ah
	out 0x8A, al
	mov ax, ssd_address_high
	out 0x8A, al
	mov al, ah
	out 0x8A, al

	mov ax, #0x80		;write control register 'erase verify'
	out 0x8C, al
	mov al, #0xA0
	out 0x8A, al
	
	mov ax, #0x87		;set VPP + WR
	out 0x8C, al
	mov al, #0x12
	out 0x8A, al
	
	sti

	mov cx, #0x0050		;delay approx 20us
loop6:
	nop
	loop loop6

	cli
	out 0x15, al

	mov ax, #0x87		;set VPP + OE
	out 0x8C, al
	mov al, #0x11
	out 0x8A, al

	mov ax, #0xC0		;verify data
	out 0x8C, al
	in al, #0x8A

	cmp al, #0xFF
	je ssd_erase_next_address

	jmp hop_skip_jump	;erase whole chip again

ssd_erase_next_address:
	mov cx, ssd_address_low

	loop ssd_erase_check	;check next address

	mov bx, ax

	mov ax, #0x87		;clear VPP + OE
	out 0x8C, al
	mov al, #0x00
	out 0x8A, al
				;don't forget to leave power and clocks on	
	
	mov ax, bx
	sti
	ret
	

;------------------------------------------------------------
;-----------------------------------------------------------
	.data

ssd_address_low:
	.word 0x0000
ssd_address_high:
	.word 0x0000
ssd_data:
	.word 0x0000

#endif
