/* Initialize Ethernet driver */

#include <linuxmt/config.h>
#include <linuxmt/init.h>

void eth_init(void)
{
#ifdef CONFIG_ETH_NE2K
	eth_drv_init();
#endif
}
