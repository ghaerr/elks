#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/wait.h>
#include <linuxmt/sched.h>
#include <linuxmt/config.h>
#include <linuxmt/fs.h> /* for ROOT_DEV */
#include <linuxmt/heap.h>

#include <arch/segment.h>


byte_t arch_cpu;  // processor number (from setup data)

#ifdef CONFIG_ARCH_SIBO
extern long int basmem;
#endif

void setup_arch(seg_t *start, seg_t *end)
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

	/* Now insert local heap at end of kernel data segment */

	heap_add (_endbss, 1 + ~ (unsigned) _endbss);

	/* TEMP: just to demonstrate the heap */

	void * h1 = heap_alloc (100);
	void * h2 = heap_alloc (100);
	heap_free (h1);
	h1 = heap_alloc (50);
	heap_free (h1);

	/* Misc */

    ROOT_DEV = setupw(0x1fc);

    arch_cpu = setupb(0x20);

}

/* Stubs for functions needed elsewhere */

void hard_reset_now(void)
{
#ifdef __BCC__
    asm(\
	"\tmov ax,#0x40\n" \
	"\tmov ds, ax\n" \
	"\tmov [0x72],#0x1234\n" \
	"\tjmp #0xffff:0\n" \
	);
#endif
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
#ifdef __BCC__
    asm(\
        "\tmov ax,#0x5301\n" \
        "\txor bx,bx\n" \
        "\tint #0x15\n" \
        "\tjc apm_error\n" \
        "\tmov ax,#0x5308\n" \
        "\tmov bx,#0x0001\n" \
        "\tmov cx,#0x0001\n" \
        "\tint #0x15\n" \
        "\tjc apm_error\n" \
        "\tmov ax,#0x5307\n" \
        "\tmov bx,#0x0001\n" \
        "\tmov cx,#0x0003\n" \
        "\tint #0x15\n" \
        "\tapm_error:\n"
        );
#endif
#ifdef __ia16__
    asm("movw $21249,%ax\n\t"
	"xorw %bx,%bx\n\t"
	"int $21\n\t"
	"jc apm_error\n\t"
	"movw $21256,%ax\n\t"
	"movw $1,%bx\n\t"
	"movw $1,%cx\n\t"
	"int $21\n\t"
	"jc apm_error\n\t"
	"movw $21255,%ax\n\t"
	"movw $1,%bx\n\t"
	"movw $3,%cx\n\t"
	"int $21\n\t"
	"apm_error:\n\t"
	);
#endif
    printk("Cannot power off: APM not supported\n");
    return;
}
#endif
