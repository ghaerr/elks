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
    int i;
    segoff_t offset;

    /* Kernel tasks can always access user process boundaries */
    if (kernel_ds == current->t_regs.ds)
        return 0;

    offset = (segoff_t)((char *)p + len);

    /* use fast method when DS == SS indicating default data segment request */
    if (current->t_regs.ds == current->t_regs.ss)
         return (offset < current->t_endseg)? 0: -EFAULT;

    /* check allocated code and data segments with syscall DS segment */
    for (i = 0; i < MAX_SEGS; i++) {
        if (current->mm[i] && (current->mm[i]->base == current->t_regs.ds) &&
           offset < (current->mm[i]->size << 4)) /* task segments never >= 64k */
            return 0;
    }

    /* check fmemalloc allocations in main memory (slow) */
    if (seg_verify_area(current->pid, current->t_regs.ds, offset))
        return 0;

    printk("FAULT\n");
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
