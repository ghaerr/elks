/*
 *	User access routines for the kernel.
 */

#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/mm.h>
#include <linuxmt/errno.h>
#include <arch/segment.h>

int verfy_area(void *p, size_t len)
{
    register __ptask currentp = current;

    /*
     *	Kernel tasks can always access
     */
    if ((kernel_ds == currentp->t_regs.ds) /* Kernel tasks can always access */
	  || ((__pptr)((char *)p + len) <= currentp->t_endseg)) /* User process boundaries */
	return 0;

    return -EFAULT;
}

int verified_memcpy_fromfs(void *daddr, void *saddr, size_t len)
{
    int err = verify_area(VERIFY_READ, saddr, len);

    if (!err)
	memcpy_fromfs(daddr, saddr, len);
    return err;
}

int verified_memcpy_tofs(void *daddr, void *saddr, size_t len)
{
    int err = verify_area(VERIFY_WRITE, daddr, len);

    if (!err)
	memcpy_tofs(daddr, saddr, len);
    return err;
}

#if 0

int fstrlen(unsigned short int dseg, unsigned short int doff)
{
    unsigned short int i = 0;

    while (peekb(dseg, doff++))
	i++;

    return i;
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
    return peekb(current->t_regs.ds, (__u16)dv);
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
