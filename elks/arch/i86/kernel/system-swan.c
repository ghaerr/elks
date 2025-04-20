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
#include <arch/irq.h>

seg_t membase, memend;  /* start and end segment of available main memory */
unsigned int heapsize;  /* max size of kernel near heap */
byte_t sys_caps;        /* system capabilities bits */
unsigned char arch_cpu; /* CPU type from cputype.S */

unsigned int INITPROC setup_arch(void)
{
    unsigned int heapofs, heapsegs;

    /*
     * Set membase to beginning of available main memory.
     * Set memend to end of available main memory.
     * If ramdisk configured, subtract space for it from end of memory.
     * Set heapsize for near heap allocator.
     * Return start address for near heap allocator.
     */

    heapofs = (unsigned int) (0x8000 - 0x0610);
    /* Start heap allocations at even addresses */
    heapofs = (heapofs + 1) & ~1;

    /* Calculate size of heap, which extends end of kernel data segment */
    heapsize = SETUP_HEAPSIZE;          /* may also be set via heap= in /bootopts */
    membase = SETUP_USERHEAPSEG;
    debug("endbss %x heap %x\n", heapofs, heapsize);

    memend = SETUP_MEM_KBYTES << 6;

    arch_cpu = SETUP_CPU_TYPE;
    sys_caps = SYS_CAPS;    /* custom system capabilities */

    return heapofs;                      /* used as start address in near heap init */
}
