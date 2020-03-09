/*
 *	Initialise the memory management. For now simply print the amount of
 *	memory!
 */

#include <linuxmt/types.h>
#include <linuxmt/kernel.h>
#include <linuxmt/config.h>

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

char proc_name[16];
__u16 kernel_cs, kernel_ds;

void mm_stat(seg_t start, seg_t end)
{
    register char *pi;
    register char *cp;

    pi = (char *)0x30;
    cp = proc_name;
    do {
	*cp++ = setupb((int)pi++);

#ifndef CONFIG_ARCH_SIBO

	if (pi == (char *) 0x40) {
	    printk("PC/%cT class machine, %s CPU\n%uK base RAM",
		    arch_cpu > 5 ? 'A' : 'X', proc_name, setupw(0x2a));
	    cp = proc_name;
	    pi = (char *) 0x50;
	}

#endif

    } while ((int)pi <
#ifndef CONFIG_ARCH_SIBO
			0x5D);
#else
			0x40);
#endif

#ifdef CONFIG_ARCH_SIBO

    printk("Psion Series 3a machine, %s CPU\n%uK base"
	    ", CPUID `NEC V30'", proc_name, basemem);

#else

    if (*proc_name)
	printk(", CPUID `%s'", proc_name);

#endif

    printk(".\nELKS kernel (%u text + %u data + %u bss)\n"
	   "Kernel text at %x:0000, data at %x:0000\n"
	   "%u K of memory for user processes.\n",
	   (unsigned)_endtext, (unsigned)_enddata,
	   (unsigned)_endbss - (unsigned)_enddata,
	   kernel_cs, kernel_ds,
	   (int) ((end - start) >> 6));

    if (setupb(0x1ff) == 0xAA && arch_cpu > 5)
	printk("ps2: PS/2 pointing device detected\n");

}
