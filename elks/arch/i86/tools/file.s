; small program to display a brief meassage on the screen
; SDW 28/2/99

	.text
	.global _open_file
	.global _read_byte
	.global _close_file

_open_file:
	mov dx, #file_status
	mov ax, #0x0000		; connect to file server.
	int 0x87
	mov cx, #0x00		; binary, readonly.
	push bp
	mov bp, sp
	mov bx, 4[bp]
	pop bp
	mov ah, #0x00
	int 0x85
	mov file_handle, ax	; Save file handle for later.
	ret

_read_byte:
	mov bx, file_handle
	mov cx, #file_buffer
	mov dx, #0x01
	mov ah, #0x11
	int 0x86
	mov ax, file_buffer
	ret

_close_file:
	mov bx, file_handle
	mov ah, #0x10
	int 0x86
	ret

	.data
file_handle:
	.word 0x0000

file_buffer:
	.byte 0x00

file_status:
	.word 0x0000
