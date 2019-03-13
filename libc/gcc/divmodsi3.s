
	.code16

	.text
	.extern	ldivmod
	.extern	ludivmod

	.global	__divsi3
	.align 1

__divsi3:
	push	%bp
	mov	%sp,%bp
	push	%bx
	push	%cx
	push	%di
	mov	4(%bp),%ax
	mov	6(%bp),%bx
	mov	8(%bp),%cx
	mov	10(%bp),%di
	call	ldivmod
	mov	%di,%dx
	mov	%cx,%ax
	pop	%di
	pop	%cx
	pop	%bx
	pop	%bp
	ret

	.global	__modsi3
	.align 1

__modsi3:
	push	%bp
	mov	%sp,%bp
	push	%bx
	push	%cx
	push	%di
	mov	4(%bp),%ax
	mov	6(%bp),%bx
	mov	8(%bp),%cx
	mov	10(%bp),%di
	call	ldivmod
	mov	%bx,%dx
	pop	%di
	pop	%cx
	pop	%bx
	pop	%bp
	ret

	.global	__udivsi3
	.align 1

__udivsi3:
	push	%bp
	mov	%sp,%bp
	push	%bx
	push	%cx
	push	%di
	mov	4(%bp),%ax
	mov	6(%bp),%bx
	mov	8(%bp),%cx
	mov	10(%bp),%di
	call	ludivmod
	mov	%di,%dx
	mov	%cx,%ax
	pop	%di
	pop	%cx
	pop	%bx
	pop	%bp
	ret

	.global	__umodsi3
	.align 1

__umodsi3:
	push	%bp
	mov	%sp,%bp
	push	%bx
	push	%cx
	push	%di
	mov	4(%bp),%ax
	mov	6(%bp),%bx
	mov	8(%bp),%cx
	mov	10(%bp),%di
	call	ludivmod
	mov	%bx,%dx
	pop	%di
	pop	%cx
	pop	%bx
	pop	%bp
	ret
