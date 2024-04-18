#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/wait.h>
#include <linuxmt/sched.h>
#include <linuxmt/fs.h> /* for ROOT_DEV */
#include <linuxmt/heap.h>
#include <linuxmt/memory.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>
#include <arch/io.h>

byte_t sys_caps;		/* system capabilities bits */
unsigned int heapsize;	/* max size of kernel near heap */

void INITPROC setup_arch(seg_t *start, seg_t *end)
{
#ifdef CONFIG_HW_COMPAQFAST
	outb_p(1,0xcf);	/* Switch COMPAQ Deskpro to high speed */
#endif

	/*
	 * Extend kernel data segment to maximum of 64K to make room
	 * for local heap.
	 *
	 * Set start to beginning of available main memory, which
	 * is directly after end of the kernel data segment.
	 *
	 * Set end to end of available main memory.
	 *
	 * If ramdisk configured, subtract space for it from end of memory.
	 */

	/* Heap allocations at even addresses, helps debugging*/
	unsigned int endbss = (unsigned int)(_endbss + 1) & ~1;

	/*
	 * Calculate size of heap, which extends end of kernel data segment
	 */

#ifdef SETUP_HEAPSIZE
	unsigned int heapsegs = (1 + ~endbss) >> 4;	/* max possible heap in segments*/
	if ((SETUP_HEAPSIZE >> 4) < heapsegs)		/* allow if less than max*/
		heapsegs = SETUP_HEAPSIZE >> 4;
	*start = kernel_ds + heapsegs + (((unsigned int) (_endbss+15)) >> 4);
	heapsize = heapsegs << 4;
#else
	*start = kernel_ds + 0x1000;
	heapsize = 1 + ~endbss;
#endif

	*end = (seg_t)SETUP_MEM_KBYTES << 6;

#if defined(CONFIG_RAMDISK_SEGMENT) && (CONFIG_RAMDISK_SEGMENT > 0)
	if (CONFIG_RAMDISK_SEGMENT <= *end) {
		/* reduce top of memory by size of ram disk*/
		*end -= CONFIG_RAMDISK_SECTORS << 5;
	}
#endif

	/* Now insert local heap at end of kernel data segment */
	heap_init ();
	heap_add ((void *)endbss, heapsize);

	/* Misc */
	ROOT_DEV = SETUP_ROOT_DEV;

#ifdef SYS_CAPS
	sys_caps = SYS_CAPS;	/* custom system capabilities */
#else
	byte_t arch_cpu = SETUP_CPU_TYPE;
	if (arch_cpu > 5)		/* IBM PC/AT capabilities */
		sys_caps = CAP_ALL;
	debug("arch %d sys_caps %02x\n", arch_cpu, sys_caps);
#endif
}

/*
 * The following routines may need porting on non-IBM PC architectures
 */

void hard_reset_now(void)
{
#ifdef CONFIG_ARCH_IBMPC
    asm("mov $0x40,%ax\n\t"
	"mov %ax,%ds\n\t"
	"movw $0x1234,0x72\n\t"
	"ljmp $0xFFFF,$0\n\t"
	);
#endif
}

/*
 *	Use Advanced Power Management to power off system
 *	For details on how this code works, see
 *	http://wiki.osdev.org/APM
 */
void apm_shutdown_now(void)
{
#ifdef CONFIG_ARCH_IBMPC
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
}
