#include <linuxmt/config.h>
#include <linuxmt/types.h>

int chr_dev_init()
{
#ifdef CONFIG_CHAR_DEV_RS
	rs_init();
#endif
#ifdef CONFIG_CHAR_DEV_LP
	lp_init();
#endif
#ifdef CONFIG_CONSOLE_DIRECT
/*	xtk_init(); */
#endif
#ifdef CONFIG_CHAR_DEV_MEM
	mem_dev_init();
#endif
#ifdef CONFIG_DEV_META
	meta_init();
#endif
}
