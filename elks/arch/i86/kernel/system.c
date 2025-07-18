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
#include <arch/system.h>
#include <arch/io.h>

seg_t membase, memend;  /* start and end segment of available main memory */
unsigned int heapsize;  /* max size of kernel near heap */
byte_t sys_caps;        /* system capabilities bits */
unsigned char arch_cpu; /* CPU type from cputype.S */

unsigned int INITPROC setup_arch(void)
{
    unsigned int endbss, heapsegs;

#ifdef CONFIG_HW_COMPAQFAST
    outb_p(1,0xcf); /* Switch COMPAQ Deskpro to high speed */
#endif

    /*
     * Extend kernel data segment to maximum of 64K to make room for local heap.
     *
     * Set membase to beginning of available main memory, which
     * is directly after end of the kernel data segment.
     *
     * Set memend to end of available main memory.
     * If ramdisk configured, subtract space for it from end of memory.
     *
     * Calculate heapsize for near heap allocator.
     * Return start address for near heap allocator.
     */

    /* Start heap allocations at even addresses */
    endbss = (unsigned int)(_endbss + 1) & ~1;

    /* Calculate size of heap, which extends end of kernel data segment */
#ifdef SETUP_HEAPSIZE
    heapsize = SETUP_HEAPSIZE;          /* may also be set via heap= in /bootopts */
#endif
    if (heapsize) {
        heapsegs = (1 + ~endbss) >> 4;  /* max possible heap in segments*/
        if ((heapsize >> 4) < heapsegs) /* allow if less than max*/
            heapsegs = heapsize >> 4;
        membase = kernel_ds + heapsegs + (((unsigned int) (_endbss+15)) >> 4);
        heapsize = heapsegs << 4;
    } else {
        membase = kernel_ds + 0x1000;
        heapsize = 1 + ~endbss;
    }
    debug("endbss %x heap %x kdata size %x\n", endbss, heapsize, (membase-kernel_ds)<<4);

    memend = SETUP_MEM_KBYTES << 6;

#if defined(CONFIG_RAMDISK_SEGMENT) && (CONFIG_RAMDISK_SEGMENT > 0)
    if (CONFIG_RAMDISK_SEGMENT <= memend) {
        /* reduce top of memory by size of ram disk*/
        memend -= CONFIG_RAMDISK_SECTORS << 5;
    }
#endif

    arch_cpu = SETUP_CPU_TYPE;
#ifdef SYS_CAPS
    sys_caps = SYS_CAPS;    /* custom system capabilities */
#else
    if (arch_cpu >= CPU_80286)          /* 80286+ IBM PC/AT capabilities or Unknown CPU */
        sys_caps = CAP_ALL;
    debug("arch %d sys_caps %02x\n", arch_cpu, sys_caps);
#endif

    return endbss;                      /* used as start address in near heap init */
}
