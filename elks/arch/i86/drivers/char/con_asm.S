
#include <linuxmt/config.h>

; Direct Console driver (assembly part)
; SDW 1st June 99

	.text

#ifdef CONFIG_SIBO_CONSOLE_DIRECT

	.global _LCD_ScrollUp
	.global _LCD_Position 
	.global _LCD_ClearLine
	.global _LCD_WriteChar
	.global _LCD_PowerOn
	.global _LCD_PowerOff

;--------------------------------------------------------------
; Position cursor to a particular location

_LCD_Position:
	push bp
	
	mov bp, sp		;get parameters
	mov ax, 4[bp]
	mov xpos, ax

	mov ax, 6[bp]		
	mov ypos, ax

	pop bp
	ret
;--------------------------------------------------------------
; Clear a line without effecting cursor position

_LCD_ClearLine:
	cli
	push bp
	
	mov bp, sp		;get line number
	mov cx, 4[bp]

	push di
	push si
	push es
	push ds

	out 0x15, al		;turn memory protection off
	
	mov ax, #0x0040
	mov es, ax		;start of screen
	mov ax, #0x0000
	
	cmp cx, #0x0000		;check for line 1 (no add!)
	je clear_positioned

clear_loop1:
	add ax, #0x01E0		;1 line's worth
	loop clear_loop1

clear_positioned:
	mov di, ax
	push ax			;store for later

;	mov cx, #0x01FE		;allow for larger last line

;	cmp ax, #0x2184		;last line
;	je clear_lastline1

	mov cx, #0x00F0		;'normal' line's worth in words

clear_lastline1:
	mov ax, #0x0000
	out 0x10, ax		;keep watchdog at bay

clear_loop2:
	stosw			;clear black plane
	loop clear_loop2

	pop ax			;retrive offset

;	mov cx, #0x01FE		;allow for larger last line

;	cmp ax, #0x2184		;last line
;	je clear_lastline2

	mov cx, #0x00F0		;'normal' line's worth in words

clear_lastline2:
	add ax, #0x2580		;offset of grey plane
	mov di, ax

	mov ax, #0x0000
	out 0x10, ax		;keep watchdog at bay

clear_loop3:
	stosw			;clear grey plane
	loop clear_loop3

	pop ds
	pop es
	pop si
	pop di
	pop bp
	sti
	ret

;--------------------------------------------------------------
; Print a character to the screen (only to black plane)
; and increase cursor position

_LCD_WriteChar:
	cli
	push bp
	
	mov bp, sp		;get parameter
	mov ah, 4[bp]

	push di
	push si
	push es
	push ds

	lea si, font_map	;locate start of font map

putchar_loop:
	lodsb			;scan for character in font map

	cmp ah, al
	je putchar_found

	add si, #0x0008		;advance to next character

	cmp al, #0xFF		;check for end of map and print '255'
	jne putchar_loop	;if wanted charcater not found	

	sub si, #0x0008		;undo what has just been added

putchar_found:
	mov di, xpos
	mov cx, ypos

	cmp cx, #0x0000
	je putchar_located

putchar_loop2:
	add di, #0x01E0		;shift down to text line
	loop putchar_loop2

putchar_located:
	mov ax, #0x0040		;LCD segment start
	mov es, ax
	mov cx, #0x0008

	out 0x15, al		;turn off memory protection

putchar_loop3:
	movsb
	add di, #0x003B		;print each pixel row in turn
	loop putchar_loop3

	inc xpos		;increase horizontal position

putchar_done:
	pop ds
	pop es
	pop si
	pop di
	pop bp

	sti
	ret


;--------------------------------------------------------------
; Scroll up one line (not including grey plane)

_LCD_ScrollUp:
	cli
	push bp
	push di
	push si
	push es
	push ds
	
scroll_up:
	out 0x15, al		;turn off memory protection

	mov ax, #0x0040		;LCD segment start
	mov ds, ax
	mov si, #0x01E0		;source line 1

	mov es, ax		;destination line 0
	mov di, #0x0000

	mov cx, #0x12C0 	;black plane (in words)
	cld
	
	out 0x10, al		;keep watchdog at bay

scroll_loop:
	movsw
	loop scroll_loop

	pop ds
	pop es
	pop si
	pop di
	pop bp

	sti
	ret

;--------------------------------------------------------------

_LCD_PowerOn:
	cli			;clear interrupts and turn off protection
	out 0x15, al		

	in ax, #0x24		;enable LCD Screen
	or ax, #0x6000
	out 0x24, ax

	mov ax, #pump_ctrl	;reset Voltage Pump settings
	out 0x2E, ax

	sti
	ret

;--------------------------------------------------------------

_LCD_PowerOff:
	cli			;clear interrupts and turn off protection
	out 0x15, al		

	in ax, #0x2E		;read current Voltage Pump Settings
	mov pump_ctrl, ax

	in ax, #0x24		;disable LCD Screen
	and ax, #0x9FFF
	out 0x24, ax

	sti
	ret

;--------------------------------------------------------------
;--------------------------------------------------------------
	.data

	.global font_map
	
;--------------------------------------------------------------
xpos:
	.word 0x0000
ypos:
	.word 0x0000
pump_ctrl:
	.word 0x0000
#endif /* CONFIG_SIBO_CONSOLE_DIRECT */
