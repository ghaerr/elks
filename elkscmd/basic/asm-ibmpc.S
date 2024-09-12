#define ARG0	2
#define ARG1	4
#define ARG2	6
#define ARG3	8
#define ARG4	10

	.arch	i8086, nojumps
	.code16
	.text

	.global fmemsetw
	.global int_10

// void fmemsetw(void * off, unsigned int seg, unsigned int val, size_t count)

fmemsetw:
	mov    %es,%bx
	mov    %di,%dx
	mov    %sp,%di
	mov    ARG2(%di),%ax  // value
	mov    ARG3(%di),%cx  // byte count
	les    ARG0(%di),%di  // far pointer
	cld
	rep
	stosw
	mov    %dx,%di
	mov    %bx,%es
	ret

// void int_10(unsigned int ax, unsigned int bx,
//             unsigned int cx, unsigned int dx)

int_10:
	push   %bp
	push   %si
	push   %di
	mov    %sp,%di
	mov    ARG0+6(%di),%ax
	mov    ARG1+6(%di),%bx
	mov    ARG2+6(%di),%cx
	mov    ARG3+6(%di),%dx
	int    $0x10
	pop    %di
	pop    %si
	pop    %bp
	ret
