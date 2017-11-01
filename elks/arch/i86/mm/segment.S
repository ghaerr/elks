/*
 *  Assembly memory access routines for the kernel.
 */
 
#include <arch/asm-offsets.h>

	.text

/*
 *  byte_t peekb (word_t off, seg_t seg);
 *  segment after offset to allow LDS from the stack
 */

	.globl _peekb

_peekb:
	push   bp
	mov    bp,sp
	push   ds
	push   bx
	lds    bx,[bp+4]           /* arg0+1: far pointer */
	mov    al,[bx]             /* DS by default */
	xor    ah,ah
	pop    bx
	pop    ds
	pop    bp
	ret

/*
 *  word_t peekw (word_t off, seg_t seg);
 *  segment after offset to allow LDS from the stack
 */

	.globl _peekw

_peekw:
	push   bp
	mov    bp,sp
	push   ds
	push   bx
	lds    bx,[bp+4]           /* arg0+1: far pointer */
	mov    ax,[bx]             /* DS by default */
	pop    bx
	pop    ds
	pop    bp
	ret

/*
 *  void pokeb (word_t off, seg_t seg, byte__t val);
 *  segment after offset to allow LDS from the stack
 *  byte_t pushed as word_t by BCC
 */

	.globl _pokeb

_pokeb:
	push   bp
	mov    bp,sp
	push   ds
	push   bx
	lds    bx,[bp+4]           /* arg0+1: far pointer */
	mov    ax,[bp+8]           /* arg2: value */
	mov    [bx],al             /* DS by default */
	pop    bx
	pop    ds
	pop    bp
	ret

/*
 *  void pokew (word_t off, seg_t seg, word_t val);
 *  segment after offset to allow LDS from the stack
 */

	.globl _pokew

_pokew:
	push   bp
	mov    bp,sp
	push   ds
	push   bx
	lds    bx,[bp+4]           /* arg0+1: far pointer */
	mov    ax,[bp+8]           /* arg2: value */
	mov    [bx],ax             /* DS by default */
	pop    bx
	pop    ds
	pop    bp
	ret

/*
 *  void fmemset (word_t off, seg_t seg, byte_t val, word_t count);
 *  segment after offset to allow LES from the stack
 *  byte_t pushed as word_t by BCC
 */

	.globl _fmemset

_fmemset:
	push   bp
	mov    bp,sp
	push   es
	push   di
	push   cx
	les    di,[bp+4]           /* arg0+1: far pointer */
	mov    ax,[bp+8]           /* arg2: value */
	mov    cx,[bx+10]          /* arg3: byte count */
	cld
	test   cx,#1               /* optimize for word multiple */
	jz     fmemset_word
	rep
	stosb
	jmp    fmemset_exit

fmemset_word:
	shr    cx,#1
	mov    ah,al
	rep
	stosw

fmemset_exit:
	pop    cx
	pop    di
	pop    es
	pop    bp
	ret

/*
 *  void fmemcpy (word_t dst_off, seg_t dst_seg, word_t src_off, seg_t src_seg, word_t count);
 *  segment after offset to allow LDS & LES from the stack
 */

	.globl _fmemcpy

_fmemcpy:
	push   bp
	mov    bp,sp
	push   es
	push   ds
	push   di
	push   si
	push   cx
	les    di,[bp+4]           /* arg0+1: far destination pointer */
	lds    si,[bp+8]           /* arg2+3: far source pointer */
	mov    cx,[bp+12]          /* arg4: byte count */
	cld
	test   cx,#1               /* optimize for word multiple */
	jz     fmemcpy_word
	rep
	movsb
	jmp    fmemcpy_exit

fmemcpy_word:
	shr    cx,#1
	rep
	movsw

fmemcpy_exit:
	pop    cx
	pop    si
	pop    di
	pop    ds
	pop    es
	pop    bp
	ret

/*
 *  int fmemcmp (word_t dst_off, seg_t dst_seg, word_t src_off, seg_t src_seg, word_t count);
 *  segment after offset to allow LDS & LES from the stack
 */

	.globl _fmemcmp

_fmemcmp:
	push   bp
	mov    bp,sp
	push   es
	push   ds
	push   di
	push   si
	push   cx
	les    di,[bp+4]           /* arg0+1: far destination pointer */
	lds    si,[bp+8]           /* arg2+3: far source pointer */
	mov    cx,[bp+12]          /* arg4: byte count */
	cld
	test   cx,#1               /* optimize for word multiple */
	jz     fmemcmp_word
	repz
	cmpsb
	jmp    fmemcmp_next

fmemcmp_word:
	shr    cx,#1
	repz
	cmpsw

fmemcmp_next:
	jz     fmemcmp_same
	mov    ax,#1
	jmp    fmemcmp_exit

fmemcmp_same:
	xor    ax,ax

fmemcpy_exit:
	pop    cx
	pop    si
	pop    di
	pop    ds
	pop    es
	pop    bp
	ret

/* TODO: move as C function in user.c */
/* void memcpy_fromfs(void *daddr, void *saddr, size_t len);*/
	.globl	_memcpy_fromfs

_memcpy_fromfs:
	mov	ax,si
	mov	dx,di
	mov	bx,sp
	mov	di,2[bx]
	mov	si,4[bx]
	mov	cx,6[bx]
	mov	bx,_current
	mov	ds,TASK_USER_DS[bx]
	mov	bx,ss
#ifdef USE_IA16
	push	es
	mov	es,bx
#endif
	cld
	rep
	movsb
#ifdef USE_IA16
	pop	es
#endif
	mov	ds,bx
	mov	di,dx
	mov	si,ax
	ret

/* TODO: move as C function in user.c */
/* void memcpy_tofs(void *daddr, void *saddr, size_t len);*/
	.globl	_memcpy_tofs

_memcpy_tofs:
	mov	ax,si
	mov	dx,di
	mov	bx,es
	mov	si,_current
	mov	es,TASK_USER_DS[si]
	mov	si,sp
	mov	di,2[si]
	mov	cx,6[si]
	mov	si,4[si]
	cld
	rep
	movsb
	mov	es,bx
	mov	di,dx
	mov	si,ax
	ret

/* TODO: revisit this one - split in generic fstrlen and user one in C */
/* int strnlen_fromfs(void *saddr, size_t maxlen); */

    /*  scasb uses es:di, not ds:si, so it is not necessary
     *  to save and restore ds
     */
	.globl	_strnlen_fromfs

_strnlen_fromfs:
	mov	dx,di
	mov	di,_current
	mov	bx,sp
	push	es
	mov	es,TASK_USER_DS[di]
	mov	di,2[bx]
	mov	cx,4[bx]
	xor	al,al		! search for NULL byte
	cld
	repne
	scasb
	pop	es
	mov	ax,di		! calc len +1
	mov	di,dx
	jnz	strnln1
	dec	ax
strnln1:
	sub	ax,2[bx]
	ret

	.data
	.extern	_current

