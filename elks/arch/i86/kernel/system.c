#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/wait.h>
#include <linuxmt/sched.h>
#include <linuxmt/config.h>
#include <linuxmt/fs.h> /* for ROOT_DEV */

#include <arch/segment.h>

int arch_cpu;			/* Processor type */
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

/*
 *      This computes the 640K - _endbss
 */

#ifndef CONFIG_ARCH_SIBO

    *end = (seg_t)((setupw(0x2a) << 6) - RAM_REDUCE);

    /* XXX plac: free root ram disk */

    *start = kernel_ds + (((unsigned int) (_endbss+15)) >> 4);

#else

    *end = (basmem)<<6;
    *start = kernel_ds + (unsigned int) 0x1000;

#endif

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
    asm("movw $64,%ax\n\t"
	"movw %ax,%ds\n\t"
	"movw $4660,114\n\t"
	"jmp 65535:0\n\t"
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
    printk("Cannot power off: APM not supported\n");
    return;
}
#endif
