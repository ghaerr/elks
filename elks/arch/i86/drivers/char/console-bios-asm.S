// API to BIOS for BIOS Console
//
	.arch	i8086, nojumps
	.code16
	.text

	.global bios_setpage
	.global bios_setcursor
	.global bios_getcursor
	.global bios_writecharattr
	.global bios_scroll

// void bios_setpage (byte_t page)
// compiler pushes byte as word

bios_setpage:
	mov    %sp,%bx
	mov    2(%bx),%al
	mov    $5,%ah		// select active display page
	int    $0x10
	ret

// void bios_setcursor (byte_t x, byte_t y, byte_t page)
// compiler pushes byte as word

bios_setcursor:
	mov    %sp,%bx
	mov    2(%bx),%dl
	mov    4(%bx),%dh
	mov    6(%bx),%bh
	mov    $2,%ah		// set cursor position
	int    $0x10
	ret

// void bios_getcursor (byte_t * x, byte_t * y)

bios_getcursor:
	mov    $3,%ah		// get cursor position
	int    $0x10
	mov    %sp,%bx
	mov    2(%bx),%bx
	mov    %dl,(%bx)
	mov    %sp,%bx
	mov    4(%bx),%bx
	mov    %dh,(%bx)
	ret

// void bios_writecharattr (byte_t c, byte_t attr, byte_t page)
// compiler pushes byte as word

bios_writecharattr:
	mov    %sp,%bx
	mov    2(%bx),%al
	mov    4(%bx),%cl
	mov    6(%bx),%bh
	mov    %cl,%bl
	mov    $1,%cx
	mov    $9,%ah		// write char and attribute
	int    $0x10
	ret

// void bios_scroll (byte_t attr, byte_t n, byte_t x, byte_t y, byte_t xx, byte_t yy)
// compiler pushes byte as word

bios_scroll:
	mov    %sp,%bx
	mov    6(%bx),%cl
	mov    8(%bx),%ch
	mov    10(%bx),%dl
	mov    12(%bx),%dh
	mov    4(%bx),%al
	mov    2(%bx),%bh
	mov    $6,%ah		// scroll active page up
	cmp    $0,%al
	jge    scroll_next
	inc    %ah		// scroll active page down
	neg    %al

scroll_next:
	int    $0x10
	ret
