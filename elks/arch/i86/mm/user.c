/*
 *	User access routines for the kernel.
 */
 
#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <arch/segment.h>
#include <linuxmt/mm.h>
#include <linuxmt/errno.h>

int verfy_area(void *p, size_t len)
{
    register char *ptr = p;
    register __ptask currentp = current;

    /*
     *	Kernel tasks can always access
     */
    if (get_ds() == currentp->t_regs.ds)
	return 0;

    /*
     *	User process boundaries
     */
    if ((__pptr)(ptr + len) > currentp->t_endseg)
	return -EFAULT;

    return 0;
}

void memcpy_fromfs(void *daddr, void *saddr, size_t len)
{
    unsigned short int ds = current->t_regs.ds;

#asm
	mov	dx,es
	mov	bx,ds
	mov	es,bx
	mov	ax,[bp-6]	! source segment (local variable)
	mov	ds,ax
	mov	di,[bp+4]	! destination address
	mov	si,[bp+6]	! source address
	mov	cx,[bp+8]	! number of bytes to copy
	cld
	rep
	movsb
	mov	ds,bx
	mov	es,dx
#endasm
}

int verified_memcpy_fromfs(void *daddr, void *saddr, size_t len)
{
    int err = verify_area(VERIFY_READ, saddr, len);

    if (err)
	return err;

    memcpy_fromfs(daddr, saddr, len);

    return 0;
}

void memcpy_tofs(void *daddr, void *saddr, size_t len)
{
    unsigned short int es = current->t_regs.ds;

#asm
	mov	dx,es
	mov	ax,[bp-6]	! source segment (local variable)
	mov	es,ax
	mov	di,[bp+4]	! destination address
	mov	si,[bp+6]	! source address
	mov	cx,[bp+8]	! number of bytes to copy
	cld
	rep
	movsb
	mov	es,dx
#endasm
}

int verified_memcpy_tofs(void *daddr, void *saddr, size_t len)
{
    unsigned short int err = verify_area(VERIFY_WRITE, daddr, len);

    if (err)
	return err;

    memcpy_tofs(daddr, saddr, len);

    return 0;
}

/* fmemcpy(dseg, dest, sseg, src, size); */

#asm	

	.globl	_fmemcpy

_fmemcpy:
	push	bp
	mov	bp, sp
	push	di
	push	si
	push	ds
	push	es
	pushf
	mov	ds, ax
	mov	si, $A[bp]
	mov	di, 6[bp]	
	mov	cx, $C[bp]
	mov	ax, 4[bp]
	mov	es, ax
	mov	ax, 8[bp]
	mov	ds, ax
	cld			! Must move upwards...
	rep
	movsb
	popf
	pop	es
	pop	ds
	pop	si
	pop	di
	pop	bp
	ret
#endasm	

#if 0

int fstrlen(unsigned short int dseg, unsigned short int doff)
{
    unsigned short int i = 0;

    while (peekb(dseg, doff++))
	i++;

    return i;
} 

#endif

#if 1

int strlen_fromfs(char *saddr)
{
    int ds = current->t_regs.ds;

    /* scasb uses es:di, not ds:si, so it is not necessary
     * to save and restore ds
     */

#asm

!	mov	bx,ds
	mov	ax,[bp-6]	! source segment (local variable)
!	mov	ds,ax
!	mov	si,[bp+4]	! source address
	mov	es,ax
	mov	di,[bp+4]	! source address
	cld
	xor	al,al		! search for NULL byte
	mov	cx,#-1
	rep
	scasb
	sub	di,[bp+4]	! calc len +1
	dec	di
	mov	[bp-6],di	! save in local var ds
!	mov	ds,bx

#endasm

    return ds;
}
#endif

unsigned long int get_fs_long(unsigned long int *dv)
{
    unsigned long retv;

    memcpy_fromfs(&retv,dv,4);

    return retv;
}

#if 0

void put_fs_long(unsigned long int dv, unsigned long int *dp)
{
    memcpy_tofs(dp,&dv,4);
}

#endif

unsigned char get_fs_byte(unsigned char *dv)
{
    unsigned char retv;

    memcpy_fromfs(&retv,dv,1);
    return retv;
}

#if 0

void put_fs_byte(unsigned char dv, unsigned char *dp)
{
    memcpy_tofs(dp,&dv,1);
}

unsigned short int get_fs_word(unsigned short int *dv)
{
    unsigned short int retv;

    memcpy_fromfs(&retv,dv,2);
    return retv;
}

void put_fs_word(unsigned short int dv, unsigned short int *dp)
{
    memcpy_tofs(dp,&dv,2);
}

#endif

int fs_memcmp(void *s, void *d, size_t len)
{
    unsigned char *p1 = s, *p2 = d;
    int c = 0;

    while (len-- && !c)
	c = peekb(current->t_regs.ds,p1++) - *p2++;

    return c;
}
