	.code16

	.text

// int poll_kbd ()

	.global poll_kbd

poll_kbd:
	mov    $1,%ah
	int    $0x16
	jnz    nhp1
	xor    %ax,%ax

nhp1:
	or     %ax,%ax
	jz     nhp2
	xor    %ah,%ah
	int    $0x16

nhp2:
	ret


// void SetDisplayPage (byte_t page)
// compiler pushes byte as word

	.global SetDisplayPage

SetDisplayPage:
	mov    %sp,%bx
	mov    2(%bx),%al
	mov    $5,%ah
	int    $0x10
	ret

// void PosCursLow (byte_t x, byte_t y, byte_t page)
// compiler pushes byte as word

	.global PosCursLow

PosCursLow:
	mov    %sp,%bx
	mov    2(%bx),%dl
	mov    4(%bx),%dh
	mov    6(%bx),%bh
	mov    $2,%ah
	int    $0x10
	ret

// void VideoWriteLow (byte_t c, byte_t attr, byte_t page)
// compiler pushes byte as word

	.global VideoWriteLow

VideoWriteLow:
	mov    %sp,%bx
	mov    2(%bx),%al
	mov    4(%bx),%cl
	mov    6(%bx),%bh
	mov    %cl,%bl
	mov    $1,%cx
	mov    $9,%ah
	int    $0x10
	ret

// void ScrollLow (byte_t attr, byte_t n, byte_t x, byte_t y, byte_t xx, byte_t yy)
// compiler pushes byte as word

	.global ScrollLow

ScrollLow:
	mov    %sp,%bx
	mov    6(%bx),%cl
	mov    8(%bx),%ch
	mov    10(%bx),%dl
	mov    12(%bx),%dh
	mov    4(%bx),%al
	mov    2(%bx),%bh
	mov    $6,%ah
	cmp    $0,%al
	jge    scroll_next
	inc    %ah
	neg    %al

scroll_next:
	int    $0x10
	ret
