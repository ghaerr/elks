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


	.globl	ldivmod
	.text
	.even

ldivmod:
	mov	dx,di		! sign byte of b in dh
	mov	dl,bh		! sign byte of a in dl
	test	di,di
	jns	set_asign
	neg	di
	neg	cx
	sbb	di,*0

set_asign:
	test	bx,bx
	jns	got_signs	! leave r = a positive
	neg	bx
	neg	ax
	sbb	bx,*0
	j	got_signs

	.globl	ludivmod
	.even

ludivmod:
	xor	dx,dx		! both sign bytes 0

got_signs:
	push	bp
	push	si
	mov	bp,sp
	push	di		! remember b
	push	cx

b0	=	-4
b16	=	-2

	test	di,di
	jne	divlarge
	test	cx,cx
	je	divzero
	cmp	bx,cx
	jae	divlarge	! would overflow
	xchg	dx,bx		! a in dx:ax, signs in bx
	div	cx
	xchg	cx,ax		! q in di:cx, junk in ax
	xchg	ax,bx		! signs in ax, junk in bx
	xchg	ax,dx		! r in ax, signs back in dx
	mov	bx,di		! r in bx:ax
	j	zdivu1

divzero:			! return q = 0 and r = a
	test	dl,dl
	jns	return
	j	negr		! a initially minus, restore it

divlarge:
	push	dx		! remember sign bytes
	mov	si,di		! w in si:dx, initially b from di:cx
	mov	dx,cx
	xor	cx,cx		! q in di:cx, initially 0
	mov	di,cx

! r in bx:ax, initially a
! use di:cx rather than dx:cx in order to
! have dx free for a byte pair later

	cmp	si,bx
	jb	loop1
	ja	zdivu		! finished if b > r
	cmp	dx,ax
	ja	zdivu

! rotate w (= b) to greatest dyadic multiple of b <= r

loop1:
	shl	dx,*1		! w = 2*w
	rcl	si,*1
	jc	loop1_exit	! w was > r counting overflow (unsigned)
	cmp	si,bx		! while w <= r (unsigned)
	jb	loop1
	ja	loop1_exit
	cmp	dx,ax
	jbe	loop1		! else exit with carry clear for rcr

loop1_exit:
	rcr	si,*1
	rcr	dx,*1

loop2:
	shl	cx,*1		! q = 2*q
	rcl	di,*1
	cmp	si,bx		! if w <= r
	jb	loop2_over
	ja	loop2_test
	cmp	dx,ax
	ja	loop2_test

loop2_over:
	add	cx,*1		! q++
	adc	di,*0
	sub	ax,dx		! r = r-w
	sbb	bx,si

loop2_test:
	shr	si,*1		! w = w/2
	rcr	dx,*1
	cmp	si,b16[bp]	! while w >= b
	ja	loop2
	jb	zdivu
	cmp	dx,b0[bp]
	jae	loop2

zdivu:
	pop	dx		! sign bytes

zdivu1:
	test	dh,dh
	js	zbminus
	test	dl,dl
	jns	return		! else a initially minus, b plus
	mov	dx,ax		! -a = b * q + r ==> a = b * (-q) + (-r)
	or	dx,bx
	je	negq		! use if r = 0
	sub	ax,b0[bp]	! use a = b * (-1 - q) + (b - r)
	sbb	bx,b16[bp]
	not	cx		! q = -1 - q (same as complement)
	not	di

negr:
	neg	bx
	neg	ax
	sbb	bx,*0
return:
	mov	sp,bp
	pop	si
	pop	bp
	ret

	.even

zbminus:
	test	dl,dl		! (-a) = (-b) * q + r ==> a = b * q + (-r)
	js	negr		! use if initial a was minus
	mov	dx,ax		! a = (-b) * q + r ==> a = b * (-q) + r
	or	dx,bx
	je	negq		! use if r = 0
	sub	ax,b0[bp]	! use a = b * (-1 - q) + (b + r) (b is now -b)
	sbb	bx,b16[bp]
	not	cx
	not	di
	mov	sp,bp
	pop	si
	pop	bp
	ret

	.even

negq:
	neg	di
	neg	cx
	sbb	di,*0
	mov	sp,bp
	pop	si
	pop	bp
	ret
