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

void setup_mm(void)
{
    long memstart, memend;
    register char *pi;
    __u16 basemem = setupw(0x2a);
    __u16 xms = setupw(2);		/* Fetched by boot code */

#ifdef CONFIG_EMS
    __u16 ems = 0;			/* Fetched by boot code */
#endif

    arch_cpu = setupb(0x20);

    for (pi = 0; ((int)pi) < 16; pi++) {
	proc_name[(int)pi] = setupb(0x30 + (int)pi);
	cpuid[(int)pi] = setupb(0x50 + (int)pi);
    }
    proc_name[16] = cpuid[16] = '\0';

#ifdef CONFIG_ARCH_SIBO

    printk("Psion Series 3a machine, %s CPU\n%dK base"
	   ", CPUID `NEC V30'",
	   proc_name, basemem, cpuid);

#else

    printk("PC/%cT class machine, %s CPU\n%dK base RAM",
	   arch_cpu > 5 ? 'A' : 'X', proc_name, basemem);
    if (arch_cpu < 6)
	xms = 0;		/* XT bios hasn't got xms interrupt */
#ifdef CONFIG_EMS
    if (ems)
	printk(", %dK expanded memory (EMS)", ems);
#endif
    if (xms)
	printk(", %dK extended memory (XMS)", xms);
    if (*cpuid)
	printk(", CPUID `%s'", cpuid);

#endif

    printk(".\nELKS kernel (%d text + %d data + %d bss)\n"
	   "Kernel text at %x:0000, data at %x:0000 \n",
	   (int) _endtext, (int) _enddata, (int) _endbss - (int) _enddata,
	   get_cs(), get_ds());

    /*
     *      This computes the 640K - _endbss 
     */

#ifdef CONFIG_ARCH_SIBO

    memend = ((long) basemem) << 10;

#else

    memend = (((long) basemem) << 10L) - (RAM_REDUCE << 4L);

#endif

    memstart = ((long) get_ds()) << 4;
    memstart += (unsigned int) _endbss + 15;

    printk("%d K of memory for user processes.\n",
	   (int) ((memend - memstart) >> 10));

    if (setupb(0x1ff) == 0xAA && arch_cpu > 5)
	printk("ps2: PS/2 pointing device detected\n");

}
