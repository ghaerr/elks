// int setupw( int *offset );
// returns the word at the far pointer SETUP_DATA:offset

#include <linuxmt/config.h>

    .code16

	.global	setupw
	.text
	.align 1

setupw:
	mov	%ds,%cx
	pop	%dx
	mov	$SETUP_DATA,%ax
	mov	%ax,%ds
	pop	%bx
	sub	$2,%sp
	mov	(%bx),%ax
	mov	%cx,%ds
	jmp	%dx
