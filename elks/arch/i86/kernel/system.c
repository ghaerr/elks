#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/wait.h>
#include <linuxmt/sched.h>
#include <linuxmt/config.h>
#include <linuxmt/fs.h> /* for ROOT_DEV */
#include <linuxmt/heap.h>
#include <linuxmt/memory.h>

#include <arch/segment.h>


byte_t arch_cpu;  // processor number (from setup data)

#ifdef CONFIG_ARCH_SIBO
extern long int basmem;
#endif

void INITPROC setup_arch(seg_t *start, seg_t *end)
{
#ifdef CONFIG_COMPAQ_FAST

/*
 *	Switch COMPAQ Deskpro to high speed
 */

    outb_p(1,0xcf);

#endif

/*
 *	Fill in the MM numbers - really ought to be in mm not kernel ?
 */

	/* Extend kernel data segment to maximum of 64K */
	/* to make room for local heap */

	/* *start = kernel_ds + (((unsigned int) (_endbss+15)) >> 4); */
	*start = kernel_ds + 0x1000;

#ifndef CONFIG_ARCH_SIBO
    *end = (seg_t)((setupw(0x2a) << 6) - RAM_REDUCE);
#else
    *end = (basmem)<<6;
#endif

	/* Heap allocations at even addresses, helps debugging*/
	unsigned int endbss = (unsigned int)(_endbss + 1) & ~1;

	/* Now insert local heap at end of kernel data segment */
	heap_init ();
	heap_add ((void *)endbss, 1 + ~endbss);

	/* Misc */
    ROOT_DEV = setupw(0x1fc);

    arch_cpu = setupb(0x20);

}

/* Stubs for functions needed elsewhere */

void hard_reset_now(void)
{
#ifdef __ia16__
    asm("mov $0x40,%ax\n\t"
	"mov %ax,%ds\n\t"
	"movw $0x1234,0x72\n\t"
	"ljmp $0xFFFF,$0\n\t"
	);
#endif
}

#ifdef CONFIG_APM
/*
 *	Use Advanced Power Management to power off system
 *	For details on how this code works, see
 *	http://wiki.osdev.org/APM
 */
void apm_shutdown_now(void)
{
#ifdef __ia16__
    asm("movw $0x5301,%ax\n\t"
	"xorw %bx,%bx\n\t"
	"int $0x15\n\t"
	"jc apm_error\n\t"
	"movw $0x5308,%ax\n\t"
	"movw $1,%bx\n\t"
	"movw $1,%cx\n\t"
	"int $0x15\n\t"
	"jc apm_error\n\t"
	"movw $0x5307,%ax\n\t"
	"movw $1,%bx\n\t"
	"movw $3,%cx\n\t"
	"int $0x15\n\t"
	"apm_error:\n\t"
	);
#endif
    printk("Cannot power off: APM not supported\n");
    return;
}
#endif
