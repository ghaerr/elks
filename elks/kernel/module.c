#include <linuxmt/config.h>

#ifdef CONFIG_MODULE
#include <linuxmt/mem.h>

char module_data[DATASIZE] = "There is no module.";

int module_init(arg)
int arg;
{
#asm
	ret
	.blkb TEXTSIZE-1
#endasm
}

#else
void module_init() {}
#endif /* CONFIG_MODULE */
