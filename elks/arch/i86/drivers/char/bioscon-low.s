
	.text

; int poll_kbd ()

	.define _poll_kbd

_poll_kbd:
	mov    ah,#1
	int    0x16
	jnz    nhp1
	xor    ax,ax

nhp1:
	or     ax,ax
	jz     nhp2
	xor    ah,ah
	int    0x16

nhp2:
	ret


; void SetDisplayPage (byte_t page)
; compiler pushes byte as word

	.define _SetDisplayPage

_SetDisplayPage:
	mov    bx,sp
	mov    al,[bx+2]
	mov    ah,#5
	int    0x10
	ret

; void PosCursLow (byte_t x, byte_t y, byte_t page)
; compiler pushes byte as word

	.define _PosCursLow

_PosCursLow:
	mov    bx,sp
	mov    dl,[bx+2]
	mov    dh,[bx+4]
	mov    bh,[bx+6]
	mov    ah,#2
	int    0x10
	ret

; void VideoWriteLow (byte_t c, byte_t attr, byte_t page)
; compiler pushes byte as word

	.define _VideoWriteLow

_VideoWriteLow:
	mov    bx,sp
	mov    al,[bx+2]
	mov    cl,[bx+4]
	mov    bh,[bx+6]
	mov    bl,cl
	mov    cx,#1
	mov    ah,#9
	int    0x10
	ret

; void ScrollLow (byte_t attr, byte_t n, byte_t x, byte_t y, byte_t xx, byte_t yy)
; compiler pushes byte as word

	.define _ScrollLow

_ScrollLow:
	mov    bx,sp
	mov    cl,[bx+6]
	mov    ch,[bx+8]
	mov    dl,[bx+10]
	mov    dh,[bx+12]
	mov    al,[bx+4]
	mov    bh,[bx+2]
	mov    ah,#6
	cmp    al,#0
	jge    scroll_next
	inc    ah
	neg    al

scroll_next:
	int    0x10
	ret
