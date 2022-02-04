/*
 *	Initialise the memory management. For now simply print the amount of
 *	memory!
 */

#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/utsname.h>
#include <linuxmt/memory.h>

#include <arch/system.h>
#include <arch/segment.h>

/*
 *	INITSEG:2	- Extended memory size
 *	INITSEG:4	- Display page
 *	INITSEG:6	- Video mode
 *	INITSEG:7	- Window Width
 *	INITSEG:8,10,12	- Video parameters
 *	INITSEG:14	- Video height (points)
 *	INITSEG:15 	- VGA present
 *	INITSEG:16	- Video Data
 *
 *	INITSEG:80	- 6 bytes of disk 0 info
 *	INITSEG:86	- 6 bytes of disk 1 info
 *
 *	INITSEG:0x01ff	- AA if psmouse present
 */

__u16 kernel_cs, kernel_ds;

void INITPROC mm_stat(seg_t start, seg_t end)
{
#ifdef CONFIG_ARCH_IBMPC
    register int i;
    register char *cp;
    static char proc_name[16];

    i = 0x30;
    cp = proc_name;
    do {
	*cp++ = setupb(i++);
	if (i == 0x40) {
	    printk("PC/%cT class machine, %s CPU, ",
		    (sys_caps & CAP_PC_AT) ? 'A' : 'X', proc_name);
	    cp = proc_name;
	    i = 0x50;
	}
    } while (i < 0x5D);
    if (*proc_name)
	printk(", CPUID `%s'", proc_name);
#endif

#ifdef CONFIG_ARCH_PC98
    printk("PC-9801 class machine, NEC CPU, ");
#endif

#ifdef CONFIG_ARCH_8018X
    printk("8018X machine, ");
#endif

    printk("%uK base RAM.\n", SETUP_MEM_KBYTES);
    printk("ELKS kernel %s (%u text, %u ftext, %u data, %u bss, %u heap)\n",
	   system_utsname.release,
	   (unsigned)_endtext, (unsigned)_endftext, (unsigned)_enddata,
	   (unsigned)_endbss - (unsigned)_enddata, heapsize);
    printk("Kernel text at %x:0000, ", kernel_cs);
#ifdef CONFIG_FARTEXT_KERNEL
    printk("ftext %x:0000, ", (unsigned)((long)kernel_init >> 16));
#endif
    printk("data %x:0000, top %x:0, %uK free\n",
	   kernel_ds, end, (int) ((end - start) >> 6));
}
