/*
 *      User access routines for the kernel.
 */

#include <linuxmt/types.h>
#include <linuxmt/sched.h>
#include <linuxmt/mm.h>
#include <linuxmt/errno.h>
#include <arch/segment.h>

int verfy_area(void *p, size_t len)
{
    /*
     * Kernel tasks can always access user process boundaries
     */
    if ((kernel_ds == current->t_regs.ds) ||
          ((segoff_t)((char *)p + len) <= current->t_endseg))
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

unsigned char get_user_char(const void *dv)
{
    return peekb((word_t)dv, current->t_regs.ds);
}

unsigned short get_user(void *dv)
{
    return peekw((word_t)dv, current->t_regs.ds);
}

unsigned long get_user_long(void *dv)
{
    return peekl((word_t)dv, current->t_regs.ds);
}

void put_user_char(unsigned char dv, void *dp)
{
    pokeb((word_t)dp, current->t_regs.ds, dv);
}

void put_user(unsigned short dv, void *dp)
{
    pokew((word_t)dp, current->t_regs.ds, dv);
}

void put_user_long(unsigned long dv, void *dp)
{
    pokel((word_t)dp, current->t_regs.ds, dv);
}

int fs_memcmp(const void *s, const void *d, size_t len)
{
    register const unsigned char *p1 = s;
    register const unsigned char *p2 = d;
    int c = 0;

    while (len-- && !c)
        c = get_user_char(p1++) - *p2++;

    return c;
}
