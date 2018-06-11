/*
 *  Assembly memory access routines for the kernel.
 */
 
#include <arch/asm-offsets.h>

    .code16

	.text

/* void memcpy_fromfs(void *daddr, void *saddr, size_t len);*/
	.global	memcpy_fromfs

memcpy_fromfs:
	mov	%si,%ax
	mov	%di,%dx
	mov	%sp,%bx
	mov	2(%bx),%di
	mov	4(%bx),%si
	mov	6(%bx),%cx
	mov	current,%bx
	mov	TASK_USER_DS(%bx),%ds
	mov	%ss,%bx
#ifdef USE_IA16
	push	%es
	mov	%bx,%es
#endif
	cld
	rep
	movsb
#ifdef USE_IA16
	pop	%es
#endif
	mov	%bx,%ds
	mov	%dx,%di
	mov	%ax,%si
	ret

/* void memcpy_tofs(void *daddr, void *saddr, size_t len);*/
	.global	memcpy_tofs

memcpy_tofs:
	mov	%si,%ax
	mov	%di,%dx
	mov	%es,%bx
	mov	current,%si
	mov	TASK_USER_DS(%si),%es
	mov	%sp,%si
	mov	2(%si),%di
	mov	6(%si),%cs
	mov	4(%si),%si
	cld
	rep
	movsb
	mov	%bx,%es
	mov	%dx,%di
	mov	%ax,%si
	ret

/* int strnlen_fromfs(void *saddr, size_t maxlen); */

    /*  scasb uses es:di, not ds:si, so it is not necessary
     *  to save and restore ds
     */
	.global	strnlen_fromfs

strnlen_fromfs:
	mov	%di,%dx
	mov	current,%di
	mov	%sp,%bx
	push	%es
	mov	TASK_USER_DS(%di),%es
	mov	2(%bx),%di
	mov	4(%bx),%cx
	xor	%al,%al		// search for NULL byte
	cld
	repne
	scasb
	pop	%es
	mov	%di,%ax		// calc len +1
	mov	%dx,%di
	jnz	strnln1
	dec	%ax
strnln1:
	sub	2(%bx),%ax
	ret

	.data
	.extern	current

