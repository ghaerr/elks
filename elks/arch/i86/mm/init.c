/*
 *	Initialise the memory management. For now simply print the amount of
 *	memory!
 */
 
#include <linuxmt/types.h>
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
 *	1ff		- AA if psmouse present
 */

char proc_name[16];
char cpuid[16];


void setup_mm()
{
	int basemem=setupw(0x2a);
	int xms=setupw(2);	/* Fetched by boot code */
	int cpu_model = setupb(0x24);
	long memstart;
	long memend;
	int i;
	arch_cpu = setupb(0x20);
	for (i = 0; i < 16; i++)
	{
		proc_name[i] = setupb(0x30 + i);
		cpuid[i] = setupb(0x50 + i);
	}
	printk("PC/%cT class machine, %s CPU\n%dK base RAM",
		arch_cpu > 5 ? 'A' : 'X', proc_name, basemem);
	if (arch_cpu<6)
		xms=0;			/* XT bios hasn't got xms interrupt */
	if (xms)
		printk(", %dK extended",xms);
	if (*cpuid)
		printk(", CPUID `%s'", cpuid);
	printk(".\nELKS kernel (%d text + %d data + %d bss)\n",
		(int)_endtext, (int)_enddata, (int)_endbss-(int)_enddata);
	printk("Kernel text at %x:0000, data at %x:0000 \n", get_cs(),
		get_ds());
	/*
	 *	This computes the 640K - _endbss 
	 */
	 
	memend = ((long)basemem)<<10;
	memstart = ((long)get_ds())<<4;
	memstart += (unsigned int)_endbss +15;
	
	printk("%d K of memory for user processes.\n",
		(int)((memend-memstart)>>10));
		
	if(setupb(0x1ff)==0xAA && arch_cpu>5)
		printk("ps2: PS/2 pointing device detected\n");

}
