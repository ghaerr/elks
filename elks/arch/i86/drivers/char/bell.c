/*
 *	elks/arch/i86/drivers/char/bell.c
 *
 *	Copyright 1999 Greg Haerr <greg@censoft.com>
 *
 * This file rings the PC speaker at a specified frequency.
 */

bell()
{
	unsigned int i;

	sound(800);
	for(i=0; i<60000; ++i)
		;
	nosound();
}

/*
 * Turn PC speaker on at passed frequency.
 */
static
sound(freq)
unsigned freq;
{
#asm
	push	bp
	mov	bp,sp

	mov	bx,  [bp+4]	! freq
	mov	ax,  #$34dd
	mov	dx,  #$0012
	cmp	dx,  bx
	jnb	none
	div	bx
	mov	bx,  ax
	in	al,  $61
	test	al,  #3
	jne	j1
	or	al,  #3
	out	$61, al
	mov	al,  #$b6
	out	$43, al
j1:
	mov	al,  bl
	out	$42, al
	mov	al,  bh
	out	$42, al
none:
	pop	bp
#endasm
}

/*
 * Turn PC speaker off.
 */
static
nosound()
{
#asm
	in	al,$61
	and	al,#$fc
	out	$61,al
#endasm
}
