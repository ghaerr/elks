#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/init.h>

void chr_dev_init(void)
{
#ifdef CONFIG_CHAR_DEV_RS
    rs_init();
#endif
#ifdef CONFIG_CHAR_DEV_LP
    lp_init();
#endif
#ifdef CONFIG_CONSOLE_BIOS
    xtk_init();
#endif
#ifdef CONFIG_CONSOLE_DIRECT
#ifndef CONFIG_ARCH_SIBO
    xtk_init();
#else
    KeyboardInit();
#endif
#endif
#ifdef CONFIG_CHAR_DEV_MEM
    mem_dev_init();
#endif
#ifdef CONFIG_DEV_META
    meta_init();
#endif
#ifdef CONFIG_INET
    tcpdev_init();
#endif
}
