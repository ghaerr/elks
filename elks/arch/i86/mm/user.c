/*
 *	User access routines for the kernel.
 */
 
#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <arch/segment.h>
#include <linuxmt/mm.h>
#include <linuxmt/errno.h>

int verfy_area(ptr,len)
char *ptr;
unsigned int len;
{
	register __ptask currentp = current;
	/*
	 *	Kernel tasks can always access
	 */
	if(get_ds()==currentp->t_regs.ds)
		return 0;
	/*
	 *	User process boundaries
	 */
	
	if((unsigned int)(ptr+len) > currentp->t_endstack)
		return -EFAULT;
		
	return 0;
}

void memcpy_fromfs(daddr, saddr, len)
char *daddr;
char *saddr;
int len;
{
	int ds = current->t_regs.ds;
#asm
        mov     dx,es
        mov     bx,ds
        mov     es,bx
        mov     ax,[bp-6]       ! source segment (local variable)
        mov     ds,ax
        mov     di,[bp+4]       ! destination address
        mov     si,[bp+6]       ! source address
        mov     cx,[bp+8]      ! number of bytes to copy
        cld
        rep
        movsb
        mov     ds,bx
        mov     es,dx
#endasm
}

int verified_memcpy_fromfs(daddr, saddr, len)
char *daddr;
register char *saddr;
int len;
{
	int err;

	if ((err = verify_area(VERIFY_READ, saddr, len)) != 0)
		return err;
	memcpy_fromfs(daddr, saddr, len);
	return 0;
}

 
void memcpy_tofs(daddr,saddr,len)
char *daddr;
char *saddr;
int len;
{
	int es = current->t_regs.ds;
#asm
        mov     dx,es
        mov     ax,[bp-6]       ! source segment (local variable)
        mov     es,ax
        mov     di,[bp+4]       ! destination address
        mov     si,[bp+6]       ! source address
        mov     cx,[bp+8]      ! number of bytes to copy
        cld
        rep
        movsb
        mov     es,dx
#endasm
}

int verified_memcpy_tofs(daddr,saddr,len)
register char *daddr;
char *saddr;
int len;
{
	int err;

	if ((err = verify_area(VERIFY_WRITE, daddr, len)) != 0)
		return err;
	memcpy_tofs(daddr, saddr, len);
	return 0;
}

#asm	
	.globl _fmemcpy
_fmemcpy:
	push bp
	mov bp, sp
	push di
	push si
	push ds
	push es
	pushf
	mov ds, ax
	mov si, $A[bp]
	mov di, 6[bp]	
	mov cx, $C[bp]
	mov ax, 4[bp]
	mov es, ax
	mov ax, 8[bp]
	mov ds, ax
	cld	! Must move upwards...
	rep 
	movsb
	popf
	pop es
	pop ds
	pop si
	pop di
	pop bp
	ret
#endasm	
#if 0
int fstrlen(dseg, doff)
unsigned int dseg, doff;
{
	int i = 0;

	while (peekb(dseg, doff++) != 0) i++;
	return i;
} 
#endif

#ifdef CONFIG_FULL_VFS
int strlen_fromfs(saddr)
char *saddr;
{
	int 	ds = current->t_regs.ds;
	/* scasb uses es:di, not ds:si, so it is not necessary to save and
	   restore ds */
#asm
!        mov     bx,ds
        mov     ax,[bp-6]       ! source segment (local variable)
!        mov     ds,ax
!        mov     si,[bp+4]       ! source address
        mov     es,ax
        mov     di,[bp+4]       ! source address
        cld
	xor	al,al		! search for NULL byte
	mov	cx,#-1
        rep
        scasb
	sub	di,[bp+4]	! calc len +1
	dec	di
	mov	[bp-6],di	! save in local var ds
!        mov     ds,bx
#endasm
	return 	ds;
}
#endif /* CONFIG_FULL_VFS */

unsigned long get_fs_long(dv)
unsigned long *dv;
{
	unsigned long retv;
	memcpy_fromfs(&retv,dv,4);
	return retv;
}

#if 0
void put_fs_long(dv,dp)
unsigned long dv;
unsigned long *dp;
{
	memcpy_tofs(dp,&dv,4);
}
#endif

unsigned char get_fs_byte(dv)
unsigned char *dv;
{
	unsigned char retv;
	memcpy_fromfs(&retv,dv,1);
	return retv;
}

#if 0
void put_fs_byte(dv,dp)
unsigned char dv;
unsigned char *dp;
{
	memcpy_tofs(dp,&dv,1);
}

unsigned int get_fs_word(dv)
unsigned int *dv;
{
	unsigned int retv;
	memcpy_fromfs(&retv,dv,2);
	return retv;
}

void put_fs_word(dv,dp)
unsigned int dv;
unsigned int *dp;
{
	memcpy_tofs(dp,&dv,2);
}
#endif
int fs_memcmp(p1,p2,len)
unsigned char *p1;
register unsigned char *p2;
int len;
{
	while(len)
	{
		unsigned char c=peekb(current->t_regs.ds,p1++);
		if(c<*p2)
			return -1;
		if(c>*p2)
			return 1;
		p2++;
		len--;
	}
	return 0;
}
