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
    register __ptask currentp = current;

    /*
     *	Kernel tasks can always access
     */
    if (kernel_ds == currentp->t_regs.ds)
	return 0;

    /*
     *	User process boundaries
     */
    if ((__pptr)((char *)p + len) > currentp->t_endseg)
	return -EFAULT;

    return 0;
}

void memcpy_fromfs(void *daddr, void *saddr, size_t len)
{
    /*@unused@*/ unsigned short int ds = current->t_regs.ds;

#ifndef S_SPLINT_S
#asm
	push	si
	push	di
	mov	dx,es
	mov	bx,ds
	mov	es,bx
	mov	di,[bp+.memcpy_fromfs.daddr]	! destination address
	mov	ds,[bp+.memcpy_fromfs.ds]	! source segment (local variable)
	mov	si,[bp+.memcpy_fromfs.saddr]	! source address
	mov	cx,[bp+.memcpy_fromfs.len]	! number of bytes to copy
	cld
	rep
	movsb
	mov	ds,bx
	mov	es,dx
	pop	di
	pop	si
#endasm
#endif

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
    /*@unused@*/ unsigned short int es = current->t_regs.ds;

#ifndef S_SPLINT_S
#asm
	push	si
	push	di
	mov	dx,es
	mov	es,[bp+.memcpy_tofs.es]	! destination segment (local variable)
	mov	di,[bp+.memcpy_tofs.daddr]	! destination address
	mov	si,[bp+.memcpy_tofs.saddr]	! source address
	mov	cx,[bp+.memcpy_tofs.len]	! number of bytes to copy
	cld
	rep
	movsb
	mov	es,dx
	pop	di
	pop	si
#endasm
#endif
}

int verified_memcpy_tofs(void *daddr, void *saddr, size_t len)
{
    int err = verify_area(VERIFY_WRITE, daddr, len);

    if (err)
	return err;

    memcpy_tofs(daddr, saddr, len);

    return 0;
}

/* fmemcpy(dseg, dest, sseg, src, size); */

#ifndef S_SPLINT_S
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
	mov	es, 4[bp]
	lds     di, 6[bp]	
	mov	si, 10[bp]
	mov	cx, 12[bp]
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
#endif

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

int strlen_fromfs(void *saddr)
{
    int ds = (int) current->t_regs.ds;

    /*  scasb uses es:di, not ds:si, so it is not necessary
     *  to save and restore ds
     */

#ifndef S_SPLINT_S
#asm

	push	di
	mov	dx,es
	mov	es,[bp+.strlen_fromfs.ds]	! source segment (local variable)
	mov	di,[bp+.strlen_fromfs.saddr]	! source address
	cld
	xor	al,al		! search for NULL byte
	mov	cx,#-1
	repne
	scasb
	sub	di,[bp+.strlen_fromfs.saddr]	! calc len +1
	dec	di
	mov	[bp+.strlen_fromfs.ds],di	! save in local var ds
	mov	es,dx
	pop	di
#endasm
#endif

    return ds;
}
#endif

unsigned long int get_user_long(void *dv)
{
    unsigned long retv;

    memcpy_fromfs(&retv,dv,4);

    return retv;
}

void put_user_long(unsigned long int dv, void *dp)
{
    memcpy_tofs(dp,&dv,4);
}

unsigned char get_user_char(void *dv)
{
    unsigned char retv;

    memcpy_fromfs(&retv,dv,1);
    return retv;
}

void put_user_char(unsigned char dv, void *dp)
{
    memcpy_tofs(dp,&dv,1);
}

#if 0

unsigned short int get_user(void *dv)
{
    unsigned short int retv;

    memcpy_fromfs(&retv,dv,2);
    return retv;
}

#endif

void put_user(unsigned short int dv, void *dp)
{
    memcpy_tofs(dp,&dv,2);
}

int fs_memcmp(void *s, void *d, size_t len)
{
    register unsigned char *p1 = s;
    register unsigned char *p2 = d;
    int c = 0;

    while (len-- && !c)
	c = get_user_char((void *)(p1++)) - *p2++;

    return c;
}
