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
 *	0x9000:2	- Extended memory size
 *	0x9000:4	- Display page
 *	0x9000:6	- Video mode
 *	0x9000:7	- Window Width
 *	0x9000:8,10,12	- Video parameters
 *	0x9000:15 	- No vga
 *	0x9000:16	- Video height (points)
 *
 *	0x9000:80	- 16 bytes of disk 0 info
 *	0x9000:90	- 16 bytes of disk 1 info
 *
 *	0x01ff		- AA if psmouse present
 */

char cpuid[17], proc_name[17];
__u16 kernel_cs, kernel_ds;

void setup_mm(seg_t start, seg_t end)
{
    register char *pi;
#ifdef CONFIG_XMS
    __u16 xms = setupw(2);		/* Fetched by boot code */
#endif

    pi = 0;
    do {
	proc_name[(int)pi] = setupb(0x30 + (int)pi);
	cpuid[(int)pi] = setupb(0x50 + (int)pi);
    } while((int)(++pi) < 16);
    proc_name[16] = cpuid[16] = '\0';

#ifdef CONFIG_ARCH_SIBO

    printk("Psion Series 3a machine, %s CPU\n%uK base"
	   ", CPUID `NEC V30'",
	   proc_name, basemem);

#else

    printk("PC/%cT class machine, %s CPU\n%uK base RAM",
	   arch_cpu > 5 ? 'A' : 'X', proc_name, setupw(0x2a));
#ifdef CONFIG_XMS
    if (arch_cpu < 6)
	xms = 0;		/* XT bios hasn't got xms interrupt */
    else
	printk(", %uK extended memory (XMS)", xms);
#endif
    if (*cpuid)
	printk(", CPUID `%s'", cpuid);

#endif

    printk(".\nELKS kernel (%u text + %u data + %u bss)\n"
	   "Kernel text at %x:0000, data at %x:0000 \n",
	   (unsigned)_endtext, (unsigned)_enddata,
	   (unsigned)_endbss - (unsigned)_enddata,
	   kernel_cs, kernel_ds);

    printk("%u K of memory for user processes.\n",
	   (int) ((end - start) >> 6));

    if (setupb(0x1ff) == 0xAA && arch_cpu > 5)
	printk("ps2: PS/2 pointing device detected\n");

}
