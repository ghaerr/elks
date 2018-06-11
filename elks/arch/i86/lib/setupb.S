// int setupb(char *offset);
// returns the (unsigned) byte at the far pointer SETUP_DATA:offset

#include <linuxmt/config.h>

    .code16

	.global	setupb
	.text
	.align 1

setupb:
	mov	%ds,%cx
	pop	%dx
	mov	$SETUP_DATA,%ax
	mov	%ax,%ds
	pop	%bx
	sub	$2,%sp
	mov	(%bx),%al
	sub	%ah,%ah
	mov	%cx,%ds
	jmp	%dx
