// strlen, strcpy, strcmp, memset routines
//
// Contributed by Christoph Niemann <niemann@swt.ruhr-uni-bochum.de>
// assume DS=SS, save ES, for GCC-IA16

#define ARG0	2
#define ARG1	4
#define ARG2	6

	.arch	i8086, nojumps
	.code16
	.text

	.global strlen
	.global strcpy
	.global strcmp
	.global memset
	.global memcpy

// int strlen(char *str)
// returns the length of string

strlen:			// needs more testing!
	mov	%es,%dx
	mov	%ss,%ax
	mov	%ax,%es
	mov	%sp,%bx
	push	%di
	mov	ARG0(%bx),%di   // str
	xor	%al,%al
	cld
	mov	$-1,%cx		// maximum loop count
	repne			// while (cx) {
	scasb			//     { cx--; if (al - [es:di++] == 0) break; }
	mov	%di,%ax
	sub	ARG0(%bx),%ax
	dec	%ax
	pop	%di
	mov	%dx,%es
	ret

// char *strcpy(char *dest, char *source)
// copies the zero terminated string source to dest

strcpy:
	mov	%es,%dx
	mov	%ss,%ax
	mov	%ax,%es
	mov	%sp,%bx
	push	%di
	push	%si
	mov	ARG0(%bx),%di	// dest
	mov	ARG1(%bx),%si	// source
	cld

cpyon:	lodsb			// al = [ds:si++]
	stosb			// [es:di++] = al
	test	%al,%al
	jnz	cpyon
	mov	ARG0(%bx),%ax	// return a pointer to the destination string
	pop	%si
	pop	%di
	mov	%dx,%es
	ret

// int strcmp(char *str1, char *str2)
// compares to zero terminated strings

strcmp:
	mov	%es,%dx
	mov	%ss,%ax
	mov	%ax,%es
	mov	%sp,%bx
	push	%di
	push	%si
	mov	ARG0(%bx),%si	// str1
	mov	ARG1(%bx),%di	// str2
	cld

cmpon:	lodsb			// al = [ds:si++]
	scasb			// al - [es:di++]
	jne	cmpend
	test	%al,%al
	jnz	cmpon
	xor	%ax,%ax		// both strings are the same
	jmp	cmpret

cmpend:	sbb	%ax,%ax		// strings differ
	or	$1,%ax

cmpret:	pop	%si
	pop	%di
	mov	%dx,%es
	ret

// char *memset(char *p, int value, size_t count)

memset:
	mov	%es,%dx
	mov	%ss,%ax
	mov	%ax,%es
	mov	%sp,%bx
	push	%di
	mov	ARG0(%bx),%di	// address of the memory block
	mov	ARG1(%bx),%ax	// byte to write
	mov	ARG2(%bx),%cx	// loop count
	cld
	shr     $1,%cx         	// store words
	mov     %al,%ah
	rep			// while (cx) cx--, [es:di++] = al
	stosw
	rcl	$1,%cx		// then possibly final byte
	rep
	stosb
	mov	ARG0(%bx),%ax	// return value = start addr of block
	pop	%di
	mov	%dx,%es
	ret

// char *memcpy(void *dest, void *src, size_t count)

memcpy:
        mov    %si,%ax
        mov    %es,%bx
        mov    %ds,%cx
        mov    %cx,%es
        mov    %di,%dx

        mov    %sp,%si
        mov    ARG2(%si),%cx  // byte count
        mov    ARG0(%si),%di  // destination pointer
        mov    ARG1(%si),%si  // source pointer
        push   %di            // save dest return value
        cld
        shr    $1,%cx         // copy words
        rep
        movsw
        rcl    $1,%cx         // then possibly final byte
        rep
        movsb

        mov    %dx,%di
        mov    %bx,%es
        mov    %ax,%si
        mov    %ss,%ax
        mov    %ax,%ds
        pop    %ax            // return dst

        ret
