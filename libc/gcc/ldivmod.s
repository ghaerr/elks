/*
! ldivmod.s - 32 over 32 to 32 bit division and remainder for 8086

! ldivmod( dividend bx:ax, divisor di:cx )  [ signed quot di:cx, rem bx:ax ]
! ludivmod( dividend bx:ax, divisor di:cx ) [ unsigned quot di:cx, rem bx:ax ]

! dx is not preserved


! NB negatives are handled correctly, unlike by the processor
! divison by zero does not trap


! let dividend = a, divisor = b, quotient = q, remainder = r
!	a = b * q + r  mod 2^32
! where:

! if b = 0, q = 0 and r = a

! otherwise, q and r are uniquely determined by the requirements:
! r has the same sign as b and absolute value smaller than that of b, i.e.
!	if b > 0, then 0 <= r < b
!	if b < 0, then 0 >= r > b
! (the absoulute value and its comparison depend on signed/unsigned)

! the rule for the sign of r means that the quotient is truncated towards
! negative infinity in the usual case of a positive divisor

! if the divisor is negative, the division is done by negating a and b,
! doing the division, then negating q and r
*/

	.code16

	.global	ldivmod
	.text
	.align 1

ldivmod:
	mov	%di,%dx		// sign byte of b in dh
	mov	%bh,%dl 	// sign byte of a in dl
	test	%di,%di
	jns	set_asign
	neg	%di
	neg	%cx
	sbb	$0,%di

set_asign:
	test	%bx,%bx
	jns	got_signs	// leave r = a positive
	neg	%bx
	neg	%ax
	sbb	$0,%bx
	jmp	got_signs

	.global	ludivmod
	.align 1

ludivmod:
	xor	%dx,%dx		// both sign bytes 0

got_signs:
	push	%bp
	push	%si
	mov	%sp,%bp
	push	%di		// remember b
	push	%cx

b0	=	-4
b16	=	-2

	test	%di,%di
	jne	divlarge
	test	%cx,%cx
	je	divzero
	cmp	%cx,%bx
	jae	divlarge	// would overflow
	xchg	%bx,%dx		// a in dx:ax, signs in bx
	div	%cx
	xchg	%ax,%cx		// q in di:cx, junk in ax
	xchg	%bx,%ax		// signs in ax, junk in bx
	xchg	%dx,%ax		// r in ax, signs back in dx
	mov	%di,%bx		// r in bx:ax
	jmp	zdivu1

divzero:			// return q = 0 and r = a
	test	%dl,%dl
	jns	return
	jmp	negr		// a initially minus, restore it

divlarge:
	push	%dx		// remember sign bytes
	mov	%di,%si		// w in si:dx, initially b from di:cx
	mov	%cx,%dx
	xor	%cx,%cx		// q in di:cx, initially 0
	mov	%cx,%di

// r in bx:ax, initially a
// use di:cx rather than dx:cx in order to
// have dx free for a byte pair later

	cmp	%bx,%si
	jb	loop1
	ja	zdivu		// finished if b > r
	cmp	%ax,%dx
	ja	zdivu

// rotate w (= b) to greatest dyadic multiple of b <= r

loop1:
	shl	$1,%dx		// w = 2*w
	rcl	$1,%si
	jc	loop1_exit	// w was > r counting overflow (unsigned)
	cmp	%bx,%si		// while w <= r (unsigned)
	jb	loop1
	ja	loop1_exit
	cmp	%ax,%dx
	jbe	loop1		// else exit with carry clear for rcr

loop1_exit:
	rcr	$1,%si
	rcr	$1,%dx

loop2:
	shl	$1,%cx		// q = 2*q
	rcl	$1,%di
	cmp	%bx,%si		// if w <= r
	jb	loop2_over
	ja	loop2_test
	cmp	%ax,%dx
	ja	loop2_test

loop2_over:
	add	$1,%cx		// q++
	adc	$0,%di
	sub	%dx,%ax		// r = r-w
	sbb	%si,%bx

loop2_test:
	shr	$1,%si		// w = w/2
	rcr	$1,%dx
	cmp	b16(%bp),%si	// while w >= b
	ja	loop2
	jb	zdivu
	cmp	b0(%bp),%dx
	jae	loop2

zdivu:
	pop	%dx		// sign bytes

zdivu1:
	test	%dh,%dh
	js	zbminus
	test	%dl,%dl
	jns	return		// else a initially minus, b plus
	mov	%ax,%dx		// -a = b * q + r ==> a = b * (-q) + (-r)
	or	%bx,%dx
	je	negq		// use if r = 0
	sub	b0(%bp),%ax	// use a = b * (-1 - q) + (b - r)
	sbb	b16(%bp),%bx
	not	%cx		// q = -1 - q (same as complement)
	not	%di

negr:
	neg	%bx
	neg	%ax
	sbb	$0,%bx
return:
	mov	%bp,%sp
	pop	%si
	pop	%bp
	ret

	.align 1

zbminus:
	test	%dl,%dl		// (-a) = (-b) * q + r ==> a = b * q + (-r)
	js	negr		// use if initial a was minus
	mov	%ax,%dx		// a = (-b) * q + r ==> a = b * (-q) + r
	or	%bx,%dx
	je	negq		// use if r = 0
	sub	b0(%bp),%ax	// use a = b * (-1 - q) + (b + r) (b is now -b)
	sbb	b16(%bp),%bx
	not	%cx
	not	%di
	mov	%bp,%sp
	pop	%si
	pop	%bp
	ret

	.align 1

negq:
	neg	%di
	neg	%cx
	sbb	$0,%di
	mov	%bp,%sp
	pop	%si
	pop	%bp
	ret
