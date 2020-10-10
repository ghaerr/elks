// memory access routines for the kernel
//
// assume DS=SS, save ES, for GCC-IA16
 
#include <arch/asm-offsets.h>

#define ARG0	2
#define ARG1	4
#define ARG2	6

	.arch	i8086, nojumps
	.code16

	.data
	.extern	current
	.text

	.global	memcpy_fromfs
	.global	memcpy_tofs
	.global	strlen_fromfs

// void memcpy_fromfs(void *daddr, void *saddr, size_t len)

memcpy_fromfs:
	mov	%si,%ax
	mov	%di,%dx
	mov	%sp,%bx
	mov	ARG0(%bx),%di		// daddr
	mov	ARG1(%bx),%si		// saddr
	mov	ARG2(%bx),%cx		// len
	mov	current,%bx
	mov	TASK_USER_DS(%bx),%ds
	mov	%ss,%bx
	push	%es
	mov	%bx,%es
	cld
	rep
	movsb
	pop	%es
	mov	%bx,%ds
	mov	%dx,%di
	mov	%ax,%si
	ret

// void memcpy_tofs(void *daddr, void *saddr, size_t len)

memcpy_tofs:
	mov	%si,%ax
	mov	%di,%dx
	mov	%es,%bx
	mov	current,%si
	mov	TASK_USER_DS(%si),%es
	mov	%sp,%si
	mov	ARG0(%si),%di		// daddr
	mov	ARG2(%si),%cx		// len
	mov	ARG1(%si),%si		// saddr
	cld
	rep
	movsb
	mov	%bx,%es
	mov	%dx,%di
	mov	%ax,%si
	ret

// int strlen_fromfs(void *saddr, size_t maxlen)
// scasb uses es:di, not ds:si, so it is not necessary to save and restore ds

strlen_fromfs:
	mov	%di,%dx
	mov	current,%di
	mov	%sp,%bx
	push	%es
	mov	TASK_USER_DS(%di),%es
	mov	ARG0(%bx),%di		// saddr
	mov	ARG1(%bx),%cx		// maxlen
	xor	%al,%al			// search for NUL byte
	cld
	repne
	scasb
	pop	%es
	mov	%di,%ax			// calc len +1
	mov	%dx,%di
	jnz	1f
	dec	%ax
1:
	sub	ARG0(%bx),%ax
	ret

