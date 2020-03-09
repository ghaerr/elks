/*
 *	Architecture specific C bootstrap
 */

#include <arch/segment.h>

extern char proc_name[15] = "Series 3a";
long int basmem = 512;

void arch_boot(void)
{
    /* setup the characteristics of the system */
}
